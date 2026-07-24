from pathlib import Path

PROJECT_DIR = Path(__file__).resolve().parent
DATASET_DIR = PROJECT_DIR / "dataset_repaired"
RAW_DATASET_DIR = PROJECT_DIR / "dataset"
OUTPUT_DIR = PROJECT_DIR / "outputs"

# Fixed class order. Keep this exact order in the ESP32 code.
CLASS_NAMES = ["neutral", "happy", "sad", "surprise"]

# Original OV3660 files are 640x480.
RAW_IMAGE_HEIGHT = 480
RAW_IMAGE_WIDTH = 640

# Tighter central crop than the previous 480x480 crop.
# This focuses the model more strongly on the face while remaining deterministic
# and easy to reproduce later on the ESP32.
CENTER_CROP_SIZE = 400

# Stronger but still ESP32-oriented MobileNetV2 candidate.
IMAGE_HEIGHT = 128
IMAGE_WIDTH = 128
CHANNELS = 3
ALPHA = 0.50

BATCH_SIZE = 16
SEED = 42
VALIDATION_SPLIT = 0.20

HEAD_EPOCHS = 35
FINE_TUNE_EPOCHS = 20
HEAD_LEARNING_RATE = 5e-4
FINE_TUNE_LEARNING_RATE = 5e-6
FINE_TUNE_LAST_LAYERS = 20

REPRESENTATIVE_SAMPLES = 240
SUPPORTED_EXTENSIONS = {".jpg", ".jpeg", ".png", ".bmp"}
