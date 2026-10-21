"""End-to-end check of the training toolchain, before any real photos exist.

Draws a small fake dataset (1280x960 JPEGs like the P4 saves: light pit floor, with fake
coins, fake clutter, or nothing), then runs train.py and quantize.py on it and checks that
a .espdl model comes out. The fake model is useless for the real box; this only proves
that the install, the pretrained weights download, training, int8 quantization and export
all work. Everything goes in smoke/ and is deleted afterwards (keep it with --keep).

    python smoke_test.py            # ~1-2 min: skips TQT (quantize.py --tqt-steps 0)
    python smoke_test.py --full     # ~20 min: quantize.py with its defaults, TQT included
"""
import argparse
import os
import shutil
import subprocess
import sys

import common  # noqa: F401  (sets OPENBLAS_NUM_THREADS before numpy loads)
import numpy as np
from PIL import Image, ImageDraw

W, H = 1280, 960
MM_PER_PX = 0.23        # full-resolution estimate at 300 mm (see BLOB_MM_PER_PX in classify_blob.c)


def floor(rng):
    base = np.array([200, 196, 188]) + rng.integers(-15, 15, size=3)
    noise = rng.normal(0, 6, size=(H, W, 1))
    return Image.fromarray(np.clip(base + noise, 0, 255).astype(np.uint8))


def center(rng):
    return W // 2 + int(rng.integers(-250, 250)), H // 2 + int(rng.integers(-200, 200))


def draw_coin(img, rng):
    d = ImageDraw.Draw(img)
    r = rng.uniform(17.9, 24.3) / 2 / MM_PER_PX
    cx, cy = center(rng)
    metal = [(176, 141, 87), (170, 170, 176), (150, 110, 80)][int(rng.integers(0, 3))]
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=metal, outline=(90, 80, 70), width=3)
    d.ellipse([cx - r * 0.6, cy - r * 0.7, cx, cy], fill=tuple(min(255, c + 40) for c in metal))   # glint


def draw_other(img, rng):
    d = ImageDraw.Draw(img)
    cx, cy = center(rng)
    color = tuple(int(c) for c in rng.integers(30, 230, size=3))
    if rng.random() < 0.5:
        w, h = rng.uniform(20, 120), rng.uniform(8, 60)
        d.rectangle([cx - w, cy - h, cx + w, cy + h], fill=color)
    else:
        pts = [(cx + float(rng.uniform(-90, 90)), cy + float(rng.uniform(-90, 90))) for _ in range(6)]
        d.polygon(pts, fill=color)


def make_dataset(root, per_class, rng):
    for name, draw in (("none", None), ("coin", draw_coin), ("other", draw_other)):
        os.makedirs(os.path.join(root, name), exist_ok=True)
        for i in range(per_class):
            img = floor(rng)
            if draw:
                draw(img, rng)
            img.save(os.path.join(root, name, f"img{i:05d}.jpg"), quality=85)


def run(*cmd):
    print("\n$ " + " ".join(cmd), flush=True)
    subprocess.run([sys.executable, *cmd], check=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--per-class", type=int, default=24)
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--full", action="store_true", help="include TQT (quantize.py defaults)")
    args = ap.parse_args()

    here = os.path.dirname(os.path.abspath(__file__))
    work = os.path.join(here, "smoke")
    data, out = os.path.join(work, "dataset"), os.path.join(work, "output")
    shutil.rmtree(work, ignore_errors=True)

    make_dataset(data, args.per_class, np.random.default_rng(0))
    run(os.path.join(here, "train.py"), "--data", data, "--out", out, "--epochs", "2")
    quantize_args = ["--data", data, "--model", os.path.join(out, "model.pt"),
                     "--out", os.path.join(out, "model.espdl"), "--calib", "72"]
    if not args.full:
        quantize_args += ["--tqt-steps", "0"]
    run(os.path.join(here, "quantize.py"), *quantize_args)

    espdl = os.path.join(out, "model.espdl")
    with open(espdl, "rb") as f:
        magic = f.read(4)
    ok = magic in (b"EDL1", b"EDL2", b"PDL1", b"PDL2", b"PDL3")
    print(f"\n{espdl}: header {magic!r} -> {'OK, toolchain works' if ok else 'NOT an ESP-DL model'}")
    if not args.keep:
        shutil.rmtree(work, ignore_errors=True)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
