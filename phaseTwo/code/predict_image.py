from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image
import tensorflow as tf

from config import CENTER_CROP_SIZE, CLASS_NAMES, IMAGE_HEIGHT, IMAGE_WIDTH, OUTPUT_DIR


def load_center_cropped_image(image_path: Path) -> np.ndarray:
    with Image.open(image_path) as image:
        image = image.convert("RGB")
        width, height = image.size
        side = min(CENTER_CROP_SIZE, width, height)
        left = (width - side) // 2
        top = (height - side) // 2
        image = image.crop((left, top, left + side, top + side))
        image = image.resize((IMAGE_WIDTH, IMAGE_HEIGHT), Image.Resampling.BILINEAR)
        array = np.asarray(image, dtype=np.float32)
    return np.expand_dims(array, axis=0)


def quantize_input(float_input: np.ndarray, details: dict) -> np.ndarray:
    scale, zero_point = details["quantization"]
    values = np.round(float_input / scale + zero_point)
    limits = np.iinfo(details["dtype"])
    return np.clip(values, limits.min, limits.max).astype(details["dtype"])


def dequantize_output(values: np.ndarray, details: dict) -> np.ndarray:
    scale, zero_point = details["quantization"]
    return (values.astype(np.float32) - zero_point) * scale


def predict_float(image: np.ndarray) -> np.ndarray:
    model_path = OUTPUT_DIR / "emotioncam_mobilenetv2_float.keras"
    model = tf.keras.models.load_model(model_path)
    return model.predict(image, verbose=0)[0]


def predict_tflite(image: np.ndarray) -> np.ndarray:
    model_path = OUTPUT_DIR / "emotioncam_mobilenetv2_int8.tflite"
    interpreter = tf.lite.Interpreter(model_path=str(model_path))
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]

    interpreter.set_tensor(
        input_details["index"],
        quantize_input(image, input_details),
    )
    interpreter.invoke()
    raw_output = interpreter.get_tensor(output_details["index"])
    return dequantize_output(raw_output, output_details)[0]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument(
        "--model",
        choices=["float", "int8"],
        default="int8",
    )
    args = parser.parse_args()

    if not args.image.is_file():
        raise FileNotFoundError(args.image)

    image = load_center_cropped_image(args.image)
    probabilities = (
        predict_float(image)
        if args.model == "float"
        else predict_tflite(image)
    )

    prediction_index = int(np.argmax(probabilities))
    print(f"Prediction: {CLASS_NAMES[prediction_index]}")
    print("Scores:")
    for class_name, score in zip(CLASS_NAMES, probabilities):
        print(f"  {class_name}: {float(score):.6f}")


if __name__ == "__main__":
    main()
