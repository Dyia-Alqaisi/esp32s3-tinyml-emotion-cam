from __future__ import annotations

import json
from pathlib import Path

import numpy as np
import tensorflow as tf
from sklearn.metrics import classification_report, confusion_matrix

from config import (
    CLASS_NAMES,
    DATASET_DIR,
    OUTPUT_DIR,
    REPRESENTATIVE_SAMPLES,
)
from dataset_utils import create_evaluation_dataset, create_representative_dataset


FLOAT_MODEL_PATH = OUTPUT_DIR / "emotioncam_mobilenetv2_float.keras"
INT8_MODEL_PATH = OUTPUT_DIR / "emotioncam_mobilenetv2_int8.tflite"


def representative_data():
    yield from create_representative_dataset(
        dataset_dir=DATASET_DIR,
        max_samples=REPRESENTATIVE_SAMPLES,
    )


def quantize_input(float_input: np.ndarray, details: dict) -> np.ndarray:
    scale, zero_point = details["quantization"]
    if scale <= 0:
        raise ValueError(f"Invalid input quantization scale: {scale}")

    quantized = np.round(float_input / scale + zero_point)
    dtype = details["dtype"]
    limits = np.iinfo(dtype)
    return np.clip(quantized, limits.min, limits.max).astype(dtype)


def dequantize_output(quantized_output: np.ndarray, details: dict) -> np.ndarray:
    scale, zero_point = details["quantization"]
    if scale <= 0:
        return quantized_output.astype(np.float32)
    return (quantized_output.astype(np.float32) - zero_point) * scale


def evaluate_tflite(model_path: Path) -> dict:
    val_ds = create_evaluation_dataset(DATASET_DIR)

    interpreter = tf.lite.Interpreter(model_path=str(model_path))
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]

    y_true: list[int] = []
    y_pred: list[int] = []

    for images, labels in val_ds:
        images_np = images.numpy().astype(np.float32)
        labels_np = labels.numpy().astype(int)

        for image, label in zip(images_np, labels_np):
            sample = np.expand_dims(image, axis=0)
            sample_quantized = quantize_input(sample, input_details)

            interpreter.set_tensor(input_details["index"], sample_quantized)
            interpreter.invoke()

            quantized_output = interpreter.get_tensor(output_details["index"])
            probabilities = dequantize_output(quantized_output, output_details)
            prediction = int(np.argmax(probabilities[0]))

            y_true.append(int(label))
            y_pred.append(prediction)

    report = classification_report(
        y_true,
        y_pred,
        labels=list(range(len(CLASS_NAMES))),
        target_names=CLASS_NAMES,
        output_dict=True,
        zero_division=0,
    )
    matrix = confusion_matrix(
        y_true,
        y_pred,
        labels=list(range(len(CLASS_NAMES))),
    )

    return {
        "model_file": str(model_path),
        "model_size_bytes": model_path.stat().st_size,
        "class_names": CLASS_NAMES,
        "input": {
            "shape": input_details["shape"].astype(int).tolist(),
            "dtype": str(input_details["dtype"]),
            "quantization_scale": float(input_details["quantization"][0]),
            "quantization_zero_point": int(input_details["quantization"][1]),
        },
        "output": {
            "shape": output_details["shape"].astype(int).tolist(),
            "dtype": str(output_details["dtype"]),
            "quantization_scale": float(output_details["quantization"][0]),
            "quantization_zero_point": int(output_details["quantization"][1]),
        },
        "classification_report": report,
        "confusion_matrix": matrix.tolist(),
    }


def main() -> None:
    if not FLOAT_MODEL_PATH.exists():
        raise FileNotFoundError(
            f"Float model not found: {FLOAT_MODEL_PATH}\n"
            "Run python train_pretrained.py first."
        )

    model = tf.keras.models.load_model(FLOAT_MODEL_PATH)

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_data

    # Require integer-only built-in operations for TensorFlow Lite Micro.
    converter.target_spec.supported_ops = [
        tf.lite.OpsSet.TFLITE_BUILTINS_INT8
    ]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8

    tflite_model = converter.convert()
    INT8_MODEL_PATH.write_bytes(tflite_model)

    results = evaluate_tflite(INT8_MODEL_PATH)
    with (OUTPUT_DIR / "int8_tflite_metrics.json").open(
        "w", encoding="utf-8"
    ) as file:
        json.dump(results, file, indent=2)

    print("Full INT8 conversion complete.")
    print(f"Model: {INT8_MODEL_PATH}")
    print(f"Size: {results['model_size_bytes'] / (1024 * 1024):.3f} MB")
    print(
        "INT8 validation accuracy: "
        f"{results['classification_report']['accuracy']:.4f}"
    )
    print("Input details:", results["input"])
    print("Output details:", results["output"])


if __name__ == "__main__":
    main()
