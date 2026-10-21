"""Pre-sort the P4's photos into dataset/<class>/ using the classifier's own guesses.

The vision board saves every deposit as imgNNNNN.jpg and appends a line per photo to
results.csv (photo_id, class, confidence, classifier). This copies each photo into the
folder of the class it was guessed as, so labeling is mostly reviewing: open each folder,
move the wrong ones to the right folder, delete junk. Photos without a guess go to unsorted/.

    python sort_photos.py --sd E:\\        # SD card in the laptop (or a folder copied off it)

Safe to re-run after collecting more photos: files already in dataset/ (in any class
folder) are skipped, so your corrections are never overwritten.
"""
import argparse
import csv
import os
import shutil

from common import CLASSES


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sd", required=True, help="SD card root, or a folder with its imgNNNNN.jpg files")
    ap.add_argument("--data", default="dataset")
    args = ap.parse_args()

    guesses = {}
    results = os.path.join(args.sd, "results.csv")
    if os.path.exists(results):
        with open(results, newline="") as f:
            for row in csv.DictReader(f):
                try:
                    guesses[int(row["photo_id"])] = CLASSES[int(row["class"])]
                except (KeyError, ValueError, IndexError):
                    continue
    else:
        print(f"no {results}: everything goes to {args.data}/unsorted/")

    folders = CLASSES + ["unsorted"]
    for name in folders:
        os.makedirs(os.path.join(args.data, name), exist_ok=True)
    already = {f for name in folders for f in os.listdir(os.path.join(args.data, name))}

    copied = {name: 0 for name in folders}
    for filename in sorted(os.listdir(args.sd)):
        if not (filename.startswith("img") and filename.endswith(".jpg")) or filename in already:
            continue
        try:
            photo_id = int(filename[3:-4])
        except ValueError:
            continue
        folder = guesses.get(photo_id, "unsorted")
        shutil.copy2(os.path.join(args.sd, filename), os.path.join(args.data, folder, filename))
        copied[folder] += 1

    print("copied: " + ", ".join(f"{name}={n}" for name, n in copied.items()))
    print(f"now review {args.data}/none, coin, other (and sort {args.data}/unsorted), then: python train.py")


if __name__ == "__main__":
    main()
