"""Transfer learning: MobileNetV2 (ImageNet weights) -> 3 CrowBox classes.

The pretrained network already knows edges, textures and shapes; only the last layer (and
optionally the last few blocks) learns "coin / nothing / other" from your photos.

    python train.py                      # dataset/ -> output/model.pt
    python train.py --epochs 25 --unfreeze 2

dataset/ layout (see sort_photos.py):  dataset/none/*.jpg  dataset/coin/*.jpg  dataset/other/*.jpg
"""
import argparse
import os
import random

import common  # noqa: F401  (sets OPENBLAS_NUM_THREADS before numpy loads)
import torch
import torch.nn as nn
from torch.utils.data import DataLoader, Subset

from common import CLASSES, DepositDataset, build_model, eval_transform, freeze_batchnorm, train_transform


def split_indices(dataset, val_frac, seed):
    """Stratified split: the same fraction of every class goes to validation."""
    by_class = {i: [] for i in range(len(CLASSES))}
    for idx, (_, target) in enumerate(dataset.samples):
        by_class[target].append(idx)
    rng = random.Random(seed)
    train_idx, val_idx = [], []
    for idxs in by_class.values():
        rng.shuffle(idxs)
        n_val = max(1, round(len(idxs) * val_frac)) if len(idxs) > 1 else 0
        val_idx += idxs[:n_val]
        train_idx += idxs[n_val:]
    return train_idx, val_idx


def evaluate(model, loader):
    model.eval()
    confusion = torch.zeros(len(CLASSES), len(CLASSES), dtype=torch.long)   # [true][predicted]
    with torch.no_grad():
        for images, targets in loader:
            preds = model(images).argmax(dim=1)
            for t, p in zip(targets, preds):
                confusion[t, p] += 1
    correct = confusion.diag().sum().item()
    total = confusion.sum().item()
    return (correct / total if total else 0.0), confusion


def print_confusion(confusion):
    print("      predicted: " + "  ".join(f"{c:>6}" for c in CLASSES))
    for i, name in enumerate(CLASSES):
        print(f"  true {name:>6}:  " + "  ".join(f"{int(v):>6}" for v in confusion[i]))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data", default="dataset")
    ap.add_argument("--out", default="output")
    ap.add_argument("--epochs", type=int, default=15)
    ap.add_argument("--batch", type=int, default=16)
    ap.add_argument("--lr", type=float, default=1e-3)
    ap.add_argument("--unfreeze", type=int, default=0,
                    help="also train the last N feature blocks (try 2 once you have a few hundred photos per class)")
    ap.add_argument("--val-frac", type=float, default=0.2)
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    torch.manual_seed(args.seed)
    torch.set_num_threads(max(1, (os.cpu_count() or 2) // 2))

    train_set = DepositDataset(args.data, transform=train_transform())
    val_set = DepositDataset(args.data, transform=eval_transform())
    counts = train_set.class_counts()
    print("photos per class: " + ", ".join(f"{c}={n}" for c, n in zip(CLASSES, counts)))
    if min(counts) == 0:
        missing = [c for c, n in zip(CLASSES, counts) if n == 0]
        raise SystemExit(f"No photos for: {missing}. Every class needs examples in {args.data}/<class>/.")

    train_idx, val_idx = split_indices(train_set, args.val_frac, args.seed)
    train_loader = DataLoader(Subset(train_set, train_idx), batch_size=args.batch, shuffle=True)
    val_loader = DataLoader(Subset(val_set, val_idx), batch_size=args.batch)
    print(f"train {len(train_idx)}, validate {len(val_idx)}")

    model = build_model(pretrained=True)
    for p in model.features.parameters():
        p.requires_grad = False
    if args.unfreeze > 0:
        for block in model.features[-args.unfreeze:]:
            for p in block.parameters():
                p.requires_grad = True

    # Weight each class by how rare it is, so a pile of empty-pit photos can't drown out coins.
    total = sum(counts)
    class_weights = torch.tensor([total / (len(CLASSES) * n) for n in counts], dtype=torch.float32)
    loss_fn = nn.CrossEntropyLoss(weight=class_weights)
    optimizer = torch.optim.AdamW([p for p in model.parameters() if p.requires_grad], lr=args.lr)
    scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=args.epochs)

    os.makedirs(args.out, exist_ok=True)
    out_path = os.path.join(args.out, "model.pt")
    best_acc = -1.0
    for epoch in range(1, args.epochs + 1):
        model.train()
        freeze_batchnorm(model)
        running = 0.0
        for images, targets in train_loader:
            optimizer.zero_grad()
            loss = loss_fn(model(images), targets)
            loss.backward()
            optimizer.step()
            running += loss.item() * len(targets)
        scheduler.step()

        acc, confusion = evaluate(model, val_loader)
        print(f"epoch {epoch:2d}/{args.epochs}  loss {running / len(train_idx):.3f}  val acc {acc:.1%}")
        if acc > best_acc:
            best_acc = acc
            torch.save({"state_dict": model.state_dict(), "classes": CLASSES,
                        "val_indices": val_idx, "val_acc": acc}, out_path)
            best_confusion = confusion

    print(f"\nbest val acc {best_acc:.1%} -> {out_path}")
    print_confusion(best_confusion)
    print("\nnext: python quantize.py")


if __name__ == "__main__":
    main()
