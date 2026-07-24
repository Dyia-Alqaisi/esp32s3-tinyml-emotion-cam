# ALQAI EmotionCam — PTQ V3 Comparison

This package keeps the selected floating-point MobileNetV2 V2 model and tests three improved post-training quantization variants.

## Variants

1. Full INT8 with Softmax and INT8 input/output
2. Full INT8 with raw logits and INT8 input/output
3. Full integer with raw logits and UINT8 input/output

All variants use all 240 images in balanced round-robin order, without augmentation, with the exact 400x400 center crop and 128x128 resize.

## What to copy

Copy the original dataset into:

```text
dataset/
├── neutral/
├── happy/
├── sad/
└── surprise/
```

Copy the selected V2 floating-point model into:

```text
outputs/emotioncam_mobilenetv2_float.keras
```

Use the exported `emotioncam_mobilenetv2_float.keras`, not `best_fine_tuned.keras`.

## Run

```powershell
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
python repair_dataset_jpegs.py
python find_corrupt_jpegs.py
python verify_dataset.py
python convert_compare_ptq_v3.py
```

## Outputs

```text
outputs/
├── emotioncam_ptq_v3_int8_softmax.tflite
├── emotioncam_ptq_v3_int8_logits.tflite
├── emotioncam_ptq_v3_uint8_logits.tflite
├── emotioncam_mobilenetv2_logits.keras
├── ptq_v3_comparison.json
└── ptq_v3_summary.csv
```

## Decision rule

- Accuracy drop up to 4 percentage points: accept PTQ
- Drop between 4 and 8 points: borderline
- Drop above 8 points: move to QAT

Send back `outputs/ptq_v3_comparison.json` and the terminal output.
