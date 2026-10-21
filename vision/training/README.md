# Vision classifier training

Turns photos from the vision board into a trained int8 MobileNetV2 that runs on the
ESP32-P4. Until a model exists, the board uses the hand-written blob classifier
(`vision/main/classify_blob.c`); switching is copying one file to the SD card.

```
P4 saves photos + guesses ─► sort_photos.py ─► review folders ─► train.py ─► quantize.py ─► SD card
   (imgNNNNN.jpg,            (dataset/none,     (move the        (model.pt)   (model.espdl)  /model.espdl
    results.csv)              coin, other)       wrong ones)
```

## Setup (done once, already done on this laptop)

esp-ppq (Espressif's quantizer) needs Python 3.12 or older, so this uses its own Python
3.12 environment in `.venv/`, separate from your default Python and from ESP-IDF's.

```powershell
cd C:\Projects\Crowbox\vision\training
py -V:3.12 -m venv .venv                                  # needs: py install 3.12
.venv\Scripts\python.exe -m pip install -r requirements.txt
.venv\Scripts\python.exe smoke_test.py                    # fake photos end to end (~1-2 min); ends "OK, toolchain works"
```

Every command below runs from `vision\training` with `.venv\Scripts\python.exe`
(or run `.venv\Scripts\Activate.ps1` first and then just `python`).

## 1. Collect

Run the box. For every deposit the vision board saves `imgNNNNN.jpg` on the SD card and
appends its guess to `results.csv`. A few hundred photos per class is a good start; the
classes are:

| folder  | means                           | firmware value |
|---------|---------------------------------|----------------|
| `none`  | pit empty                       | `CLASS_NONE`   |
| `coin`  | a coin                          | `CLASS_COIN`   |
| `other` | anything else (trash, sticks)   | `CLASS_OTHER`  |

Empty-pit and junk photos matter as much as coins: the model only learns what "not a coin"
looks like from examples of it. `none` fills itself: after every trapdoor clear the board
photographs the empty pit (its background reference) and logs it as a known `none`. Staging
deposits yourself (coins and junk, different positions) fills `coin` and `other` much
faster than waiting for crows.

## 2. Sort

```powershell
.venv\Scripts\python.exe sort_photos.py --sd E:\      # E: = the SD card
```

Copies each photo into `dataset/<what the classifier guessed>/`. Open the three folders,
move the wrong ones, delete junk, and sort `dataset/unsorted/`. Re-running later only adds
new photos; it never touches ones already in `dataset/`.

## 3. Train

```powershell
.venv\Scripts\python.exe train.py
```

A few minutes on the CPU. Prints validation accuracy each epoch and a confusion table at
the end (rows = true class, columns = predicted). With a few hundred photos per class, try
`--unfreeze 2` (also retrains the last two feature blocks) and `--epochs 25`.

## 4. Quantize

```powershell
.venv\Scripts\python.exe quantize.py
```

Converts `output/model.pt` to int8 `output/model.espdl` (about 2.5 MB) and prints float
vs int8 accuracy. Takes ~15-25 minutes: after calibrating, it fine-tunes the int8 scales
block by block (TQT), which is what keeps int8 close to float. `--tqt-steps 0` skips that
for a quick, rougher model while experimenting. If int8 still lands more than a few points
below float, the calibration photos don't cover what the box sees: add more varied photos
(see `--help` for the other knobs).

## 5. Deploy

Copy `output/model.espdl` to the SD card root as **`model.espdl`**, put the card back, and
reset the P4. Its log says `classifier: model`. To go back to the blob classifier, delete
the file.

The main ESP32 only acts on vision results once `stage1_validation_strictness` >= 3
(`CV_STRICTNESS_LEVEL` in `src/comms/cv.c`); below that, weight alone decides.

## What must stay in sync with the firmware

`common.py` holds everything training and the board have to agree on:

- `CLASSES` order is `UART_class_enum` in `shared/vision_protocol.h`.
- `device_crop_resize()` matches what ESP-DL does on the P4: center square crop of the
  1280x960 frame, nearest-neighbor resize to 224x224.
- `MEAN` / `STD` match `kMean` / `kStd` in `vision/main/classify_model.cpp` (there x 255).
