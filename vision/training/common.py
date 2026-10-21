"""Shared settings for training and quantizing the CrowBox deposit classifier.

Everything here has to match the vision firmware (vision/main/classify_model.cpp):
  - CLASSES order == UART_class_enum in shared/vision_protocol.h: the model's output
    index i is sent to the main ESP32 as class i.
  - device_crop_resize() == what ESP-DL's ImagePreprocessor does to the camera frame on
    the P4: center square crop, then nearest-neighbor resize to INPUT_SIZE.
  - MEAN / STD == what classify_model.cpp passes to ImagePreprocessor (there x 255).
Change one side, change the other.
"""
import os

# This laptop: OpenBLAS (inside numpy) reserves a buffer per CPU thread at import and
# can fail outright when memory is tight. A few threads is plenty here.
os.environ.setdefault("OPENBLAS_NUM_THREADS", "4")

import numpy as np
import torch
import torch.nn as nn
import torchvision
from PIL import Image
from torchvision import datasets, transforms
from torchvision.models import MobileNet_V2_Weights

CLASSES = ["none", "coin", "other"]   # index == UART_class_enum: CLASS_NONE, CLASS_COIN, CLASS_OTHER
INPUT_SIZE = 224
MEAN = [0.485, 0.456, 0.406]          # ImageNet statistics, which the pretrained weights expect
STD = [0.229, 0.224, 0.225]


def device_crop_resize(img: Image.Image) -> Image.Image:
    """Center square crop + nearest-neighbor resize, index-for-index like ESP-DL's resize_nn."""
    img = img.convert("RGB")
    w, h = img.size
    side = min(w, h)
    x0 = (w - side) // 2
    y0 = (h - side) // 2
    # ESP-DL: src = min(int(i * (side / size)), side - 1) + crop offset
    idx = np.minimum((np.arange(INPUT_SIZE) * (side / INPUT_SIZE)).astype(np.int64), side - 1)
    pixels = np.asarray(img)
    return Image.fromarray(pixels[y0 + idx][:, x0 + idx])


class RandomRot90:
    """Rotate by 0/90/180/270 degrees. The camera looks straight down, so any rotation of a
    deposit is equally real, and right angles add no black corners."""

    def __call__(self, img: Image.Image) -> Image.Image:
        k = int(torch.randint(0, 4, (1,)))
        return img if k == 0 else img.transpose([Image.ROTATE_90, Image.ROTATE_180, Image.ROTATE_270][k - 1])


def train_transform():
    return transforms.Compose([
        transforms.Lambda(device_crop_resize),
        RandomRot90(),
        transforms.RandomHorizontalFlip(),
        transforms.ColorJitter(brightness=0.25, contrast=0.25, saturation=0.15),
        transforms.ToTensor(),
        transforms.Normalize(MEAN, STD),
    ])


def eval_transform():
    return transforms.Compose([
        transforms.Lambda(device_crop_resize),
        transforms.ToTensor(),
        transforms.Normalize(MEAN, STD),
    ])


class DepositDataset(datasets.ImageFolder):
    """ImageFolder with the class order fixed to CLASSES (plain ImageFolder sorts folder names
    alphabetically, which would scramble the class numbers the firmware sends)."""

    def find_classes(self, directory):
        return list(CLASSES), {name: i for i, name in enumerate(CLASSES)}

    def __init__(self, root, transform=None):
        super().__init__(root, transform=transform, allow_empty=True)

    def class_counts(self):
        counts = [0] * len(CLASSES)
        for _, target in self.samples:
            counts[target] += 1
        return counts


def relu6_to_relu(module: nn.Module) -> nn.Module:
    """ReLU instead of ReLU6, as in Espressif's MobileNetV2 quantization recipe (needed for
    quantize.py --equalize, and quantized better than ReLU6 in our tests). Swapped before
    training, so the classifier head trains on the same features that get quantized."""
    for name, child in module.named_children():
        if isinstance(child, nn.ReLU6):
            setattr(module, name, nn.ReLU())
        else:
            relu6_to_relu(child)
    return module


def build_model(pretrained: bool = True) -> nn.Module:
    weights = MobileNet_V2_Weights.IMAGENET1K_V1 if pretrained else None
    model = torchvision.models.mobilenet_v2(weights=weights)   # downloads ~14 MB the first time
    relu6_to_relu(model)
    model.classifier[1] = nn.Linear(model.last_channel, len(CLASSES))
    return model


def freeze_batchnorm(model: nn.Module) -> None:
    """Keep BatchNorm running statistics from ImageNet: small batches would wreck them."""
    for m in model.modules():
        if isinstance(m, nn.BatchNorm2d):
            m.eval()
