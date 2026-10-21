"""Quantize the trained model to int8 for the ESP32-P4 and export it as .espdl.

    python quantize.py                   # output/model.pt -> output/model.espdl

Calibration: ESP-PPQ runs your photos through the float model to measure each layer's
value range, then picks the int8 scale (a power of two) for every tensor. TQT then
fine-tunes those scales block by block against the float model's outputs: the slow part
(~15-25 min on this laptop's CPU) and the part that recovers the accuracy int8 loses
(smoke-test model: 67% -> 100%). Afterwards it compares float vs int8 accuracy on the
validation photos train.py held out.

    --tqt-steps 0         skip TQT: a quick (less accurate) model for trying things out
    --calib 128           calibration photos; TQT keeps every one's activations in RAM,
                          so going much higher can run this laptop out of memory
    --int16-head          last layer (3 outputs) in 16-bit: costs nothing on the P4
    --equalize            layerwise equalization (helped Espressif on ImageNet; hurt in our tests)
    --report              per-layer error tables: which layers lose the most to int8

Deploy: copy output/model.espdl to the root of the P4's SD card as model.espdl and reboot.
"""
import argparse
import math
import os
import random

import common  # noqa: F401  (sets OPENBLAS_NUM_THREADS before numpy loads)
import torch
from torch.utils.data import DataLoader, Subset

from common import CLASSES, INPUT_SIZE, DepositDataset, build_model, eval_transform

HEAD_LAYER = "/classifier/classifier.1/Gemm"     # the final Linear, as named in the exported graph


def images_only(batch):
    return torch.stack([image for image, _ in batch])


def accuracy(forward, dataset, indices):
    correct = 0
    with torch.no_grad():
        for i in indices:
            image, target = dataset[i]
            correct += int(forward(image[None]).argmax(dim=1).item() == target)
    return correct / len(indices) if indices else 0.0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data", default="dataset")
    ap.add_argument("--model", default=os.path.join("output", "model.pt"))
    ap.add_argument("--out", default=os.path.join("output", "model.espdl"))
    ap.add_argument("--calib", type=int, default=128, help="max calibration photos (more = steadier, more RAM)")
    ap.add_argument("--batch", type=int, default=8)
    ap.add_argument("--target", default="esp32p4")
    ap.add_argument("--int16-head", action="store_true", help="run the final layer in 16-bit")
    ap.add_argument("--tqt-steps", type=int, default=300, help="TQT scale fine-tuning steps per block (0 = off)")
    ap.add_argument("--equalize", action="store_true", help="layerwise equalization")
    ap.add_argument("--report", action="store_true",
                    help="print ESP-PPQ's per-layer quantization error tables (adds ~5 min)")
    args = ap.parse_args()

    # Imported here: esp_ppq takes a while to load, and --help shouldn't wait for it.
    from esp_ppq import QuantizationSettingFactory
    from esp_ppq.api import espdl_quantize_torch, get_target_platform
    from esp_ppq.executor import TorchExecutor

    checkpoint = torch.load(args.model, map_location="cpu")
    if checkpoint["classes"] != CLASSES:
        raise SystemExit(f"model was trained with classes {checkpoint['classes']}, common.py has {CLASSES}")
    model = build_model(pretrained=False)
    model.load_state_dict(checkpoint["state_dict"])
    model.eval()

    dataset = DepositDataset(args.data, transform=eval_transform())
    calib_idx = list(range(len(dataset)))
    random.Random(0).shuffle(calib_idx)
    calib_idx = calib_idx[:args.calib]
    calib_loader = DataLoader(Subset(dataset, calib_idx), batch_size=args.batch, collate_fn=images_only)

    setting = QuantizationSettingFactory.espdl_setting()    # KL calibration for activations
    if args.int16_head:
        setting.dispatching_table.append(HEAD_LAYER, get_target_platform(args.target, 16))
    if args.tqt_steps > 0:
        setting.tqt_optimization = True
        setting.tqt_optimization_setting.steps = args.tqt_steps
        setting.tqt_optimization_setting.lr = 5e-5          # Espressif's MobileNetV2 value for the P4
        setting.tqt_optimization_setting.collecting_device = "cpu"
    if args.equalize:
        setting.equalization = True
        setting.equalization_setting.iterations = 6
        setting.equalization_setting.value_threshold = 0.5
        setting.equalization_setting.opt_level = 2
        setting.equalization_setting.interested_layers = None

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    graph = espdl_quantize_torch(
        model=model,
        espdl_export_file=args.out,
        calib_dataloader=calib_loader,
        calib_steps=math.ceil(len(calib_idx) / args.batch),
        input_shape=[1, 3, INPUT_SIZE, INPUT_SIZE],
        target=args.target,
        num_of_bits=8,
        collate_fn=lambda batch: batch.to("cpu"),
        setting=setting,
        device="cpu",
        error_report=args.report,
        skip_export=False,
        export_test_values=False,
        verbose=1,
    )

    val_idx = checkpoint.get("val_indices") or calib_idx
    executor = TorchExecutor(graph=graph, device="cpu")
    float_acc = accuracy(model, dataset, val_idx)
    int8_acc = accuracy(lambda x: torch.as_tensor(executor(x)[0]), dataset, val_idx)
    size_kb = os.path.getsize(args.out) / 1024
    print(f"\nvalidation accuracy ({len(val_idx)} photos): float {float_acc:.1%}, int8 {int8_acc:.1%}")
    if float_acc - int8_acc > 0.03:
        print("int8 lost more than 3 points: try --tqt-steps 600, or --int16-head (see --help); "
              "more varied photos help most")
    print(f"{args.out}: {size_kb:.0f} KB")
    print("deploy: copy it to the SD card root as model.espdl, then reboot the P4 "
          "(its log says 'classifier: model').")


if __name__ == "__main__":
    main()
