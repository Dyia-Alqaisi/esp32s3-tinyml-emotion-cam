from __future__ import annotations

import csv
import json
from pathlib import Path
import numpy as np
import tensorflow as tf
from sklearn.metrics import classification_report, confusion_matrix
from config import CLASS_NAMES, DATASET_DIR, OUTPUT_DIR, REPRESENTATIVE_SAMPLES
from dataset_utils import create_evaluation_dataset, create_representative_dataset, validate_dataset_structure

FLOAT_MODEL_PATH = OUTPUT_DIR / "emotioncam_mobilenetv2_float.keras"


def representative_data():
    yield from create_representative_dataset(dataset_dir=DATASET_DIR, max_samples=REPRESENTATIVE_SAMPLES)


def build_logits_model(softmax_model: tf.keras.Model) -> tf.keras.Model:
    softmax_layer = softmax_model.get_layer("expression_probabilities")
    logits_layer = tf.keras.layers.Dense(len(CLASS_NAMES), activation=None, name="expression_logits")
    logits = logits_layer(softmax_layer.input)
    logits_model = tf.keras.Model(inputs=softmax_model.input, outputs=logits, name="alqai_emotioncam_logits")
    logits_layer.set_weights(softmax_layer.get_weights())
    return logits_model


def convert_full_integer(model: tf.keras.Model, output_path: Path, io_dtype: type) -> None:
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_data
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = io_dtype
    converter.inference_output_type = io_dtype
    output_path.write_bytes(converter.convert())


def quantize_input(values: np.ndarray, details: dict) -> np.ndarray:
    scale, zero_point = details["quantization"]
    if scale <= 0:
        raise ValueError(f"Invalid input quantization scale: {scale}")
    quantized = np.round(values / scale + zero_point)
    limits = np.iinfo(details["dtype"])
    return np.clip(quantized, limits.min, limits.max).astype(details["dtype"])


def dequantize_output(values: np.ndarray, details: dict) -> np.ndarray:
    scale, zero_point = details["quantization"]
    if scale <= 0:
        return values.astype(np.float32)
    return (values.astype(np.float32) - zero_point) * scale


def metrics_from_predictions(y_true: list[int], y_pred: list[int]) -> dict:
    report = classification_report(y_true, y_pred, labels=list(range(len(CLASS_NAMES))), target_names=CLASS_NAMES, output_dict=True, zero_division=0)
    matrix = confusion_matrix(y_true, y_pred, labels=list(range(len(CLASS_NAMES))))
    return {"accuracy": float(report["accuracy"]), "classification_report": report, "confusion_matrix": matrix.tolist()}


def evaluate_keras(model: tf.keras.Model, val_ds: tf.data.Dataset) -> dict:
    y_true, y_pred = [], []
    for images, labels in val_ds:
        outputs = model.predict(images, verbose=0)
        y_true.extend(labels.numpy().astype(int).tolist())
        y_pred.extend(np.argmax(outputs, axis=1).astype(int).tolist())
    return metrics_from_predictions(y_true, y_pred)


def evaluate_tflite(model_path: Path, val_ds: tf.data.Dataset) -> dict:
    interpreter = tf.lite.Interpreter(model_path=str(model_path))
    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]
    y_true, y_pred = [], []
    for images, labels in val_ds:
        images_np = images.numpy().astype(np.float32)
        labels_np = labels.numpy().astype(int)
        for image, label in zip(images_np, labels_np):
            sample = np.expand_dims(image, axis=0)
            interpreter.set_tensor(input_details["index"], quantize_input(sample, input_details))
            interpreter.invoke()
            raw_output = interpreter.get_tensor(output_details["index"])
            output = dequantize_output(raw_output, output_details)
            y_true.append(int(label))
            y_pred.append(int(np.argmax(output[0])))
    result = metrics_from_predictions(y_true, y_pred)
    result.update({
        "model_size_bytes": model_path.stat().st_size,
        "input": {"shape": input_details["shape"].astype(int).tolist(), "dtype": str(input_details["dtype"]), "quantization_scale": float(input_details["quantization"][0]), "quantization_zero_point": int(input_details["quantization"][1])},
        "output": {"shape": output_details["shape"].astype(int).tolist(), "dtype": str(output_details["dtype"]), "quantization_scale": float(output_details["quantization"][0]), "quantization_zero_point": int(output_details["quantization"][1])},
    })
    return result


def select_recommendation(results: dict[str, dict]) -> str:
    quantized_names = ["int8_softmax", "int8_logits", "uint8_logits"]
    best_name = max(quantized_names, key=lambda name: results[name]["accuracy"])
    best_accuracy = results[best_name]["accuracy"]
    float_accuracy = results["float_softmax"]["accuracy"]
    drop = float_accuracy - best_accuracy
    verdict = "ACCEPT_PTQ" if drop <= 0.04 else ("BORDERLINE_PTQ" if drop <= 0.08 else "MOVE_TO_QAT")
    return f"{verdict}: best={best_name}, float_accuracy={float_accuracy:.4f}, best_quantized_accuracy={best_accuracy:.4f}, drop={drop:.4f}"


def save_summary_csv(results: dict[str, dict]) -> None:
    with (OUTPUT_DIR / "ptq_v3_summary.csv").open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["model", "accuracy", "size_bytes", "input_dtype", "output_dtype"])
        for name, result in results.items():
            writer.writerow([name, result["accuracy"], result.get("model_size_bytes", ""), result.get("input", {}).get("dtype", ""), result.get("output", {}).get("dtype", "")])


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    validate_dataset_structure(DATASET_DIR)
    if not FLOAT_MODEL_PATH.exists():
        raise FileNotFoundError(f"Missing float model: {FLOAT_MODEL_PATH}\nCopy your selected emotioncam_mobilenetv2_float.keras into outputs.")

    softmax_model = tf.keras.models.load_model(FLOAT_MODEL_PATH)
    logits_model = build_logits_model(softmax_model)
    logits_model.save(OUTPUT_DIR / "emotioncam_mobilenetv2_logits.keras")

    variants = {
        "int8_softmax": (softmax_model, OUTPUT_DIR / "emotioncam_ptq_v3_int8_softmax.tflite", tf.int8),
        "int8_logits": (logits_model, OUTPUT_DIR / "emotioncam_ptq_v3_int8_logits.tflite", tf.int8),
        "uint8_logits": (logits_model, OUTPUT_DIR / "emotioncam_ptq_v3_uint8_logits.tflite", tf.uint8),
    }
    for name, (model, path, dtype) in variants.items():
        print(f"\nConverting {name}...")
        convert_full_integer(model, path, dtype)
        print(f"Saved: {path}")

    results = {
        "float_softmax": evaluate_keras(softmax_model, create_evaluation_dataset(DATASET_DIR)),
        "float_logits": evaluate_keras(logits_model, create_evaluation_dataset(DATASET_DIR)),
    }
    for name, (_, path, _) in variants.items():
        results[name] = evaluate_tflite(path, create_evaluation_dataset(DATASET_DIR))

    recommendation = select_recommendation(results)
    payload = {"class_names": CLASS_NAMES, "representative_samples": REPRESENTATIVE_SAMPLES, "results": results, "recommendation": recommendation}
    (OUTPUT_DIR / "ptq_v3_comparison.json").write_text(json.dumps(payload, indent=2), encoding="utf-8")
    save_summary_csv(results)

    print("\nPTQ V3 comparison")
    for name, result in results.items():
        size = result.get("model_size_bytes")
        size_text = f", size={size/(1024*1024):.3f} MiB" if size is not None else ""
        print(f"- {name}: accuracy={result['accuracy']:.4f}{size_text}")
    print("\nRecommendation:")
    print(recommendation)
    print("\nSend back outputs/ptq_v3_comparison.json and the terminal output.")


if __name__ == "__main__":
    main()
