from __future__ import annotations

import csv
import json
import os
import random
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import tensorflow as tf
from sklearn.metrics import classification_report, confusion_matrix, ConfusionMatrixDisplay

from config import (
    ALPHA,
    CENTER_CROP_SIZE,
    CHANNELS,
    CLASS_NAMES,
    DATASET_DIR,
    FINE_TUNE_EPOCHS,
    FINE_TUNE_LAST_LAYERS,
    FINE_TUNE_LEARNING_RATE,
    HEAD_EPOCHS,
    HEAD_LEARNING_RATE,
    IMAGE_HEIGHT,
    IMAGE_WIDTH,
    OUTPUT_DIR,
    SEED,
)
from dataset_utils import create_datasets, validate_dataset_structure


def set_reproducible_seed(seed: int) -> None:
    os.environ["PYTHONHASHSEED"] = str(seed)
    random.seed(seed)
    np.random.seed(seed)
    tf.random.set_seed(seed)


def build_model() -> tuple[tf.keras.Model, tf.keras.Model]:
    input_shape = (IMAGE_HEIGHT, IMAGE_WIDTH, CHANNELS)

    base_model = tf.keras.applications.MobileNetV2(
        input_shape=input_shape,
        alpha=ALPHA,
        include_top=False,
        weights="imagenet",
    )
    base_model.trainable = False

    inputs = tf.keras.Input(shape=input_shape, dtype=tf.float32, name="image")
    x = tf.keras.layers.Rescaling(
        scale=1.0 / 127.5,
        offset=-1.0,
        name="mobilenetv2_preprocess",
    )(inputs)

    # Keep MobileNetV2 BatchNormalization layers in inference mode.
    x = base_model(x, training=False)
    x = tf.keras.layers.GlobalAveragePooling2D(name="global_average_pool")(x)
    x = tf.keras.layers.Dense(64, activation="relu", name="expression_features")(x)
    x = tf.keras.layers.Dropout(0.30, name="dropout")(x)
    outputs = tf.keras.layers.Dense(
        len(CLASS_NAMES),
        activation="softmax",
        name="expression_probabilities",
    )(x)

    model = tf.keras.Model(inputs, outputs, name="alqai_emotioncam_mobilenetv2_v2")
    return model, base_model


def compile_model(model: tf.keras.Model, learning_rate: float) -> None:
    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=learning_rate),
        loss=tf.keras.losses.SparseCategoricalCrossentropy(),
        metrics=[tf.keras.metrics.SparseCategoricalAccuracy(name="accuracy")],
    )


def callbacks_for(stage_name: str) -> list[tf.keras.callbacks.Callback]:
    checkpoint = OUTPUT_DIR / f"best_{stage_name}.keras"

    # ModelCheckpoint and EarlyStopping now monitor the same metric.
    return [
        tf.keras.callbacks.ModelCheckpoint(
            str(checkpoint),
            monitor="val_accuracy",
            mode="max",
            save_best_only=True,
            verbose=1,
        ),
        tf.keras.callbacks.EarlyStopping(
            monitor="val_accuracy",
            mode="max",
            min_delta=0.002,
            patience=8,
            restore_best_weights=True,
            verbose=1,
        ),
        tf.keras.callbacks.ReduceLROnPlateau(
            monitor="val_loss",
            mode="min",
            factor=0.30,
            patience=3,
            min_lr=1e-7,
            verbose=1,
        ),
    ]


def merge_histories(*histories: tf.keras.callbacks.History) -> dict[str, list[float]]:
    merged: dict[str, list[float]] = {}
    for history in histories:
        for key, values in history.history.items():
            merged.setdefault(key, []).extend(float(value) for value in values)
    return merged


def save_training_curves(history: dict[str, list[float]]) -> None:
    epochs = range(1, len(history.get("loss", [])) + 1)

    plt.figure(figsize=(8, 5))
    plt.plot(epochs, history.get("accuracy", []), label="Training accuracy")
    plt.plot(epochs, history.get("val_accuracy", []), label="Validation accuracy")
    plt.xlabel("Epoch")
    plt.ylabel("Accuracy")
    plt.title("MobileNetV2 V2 Accuracy")
    plt.legend()
    plt.tight_layout()
    plt.savefig(OUTPUT_DIR / "training_accuracy.png", dpi=180)
    plt.close()

    plt.figure(figsize=(8, 5))
    plt.plot(epochs, history.get("loss", []), label="Training loss")
    plt.plot(epochs, history.get("val_loss", []), label="Validation loss")
    plt.xlabel("Epoch")
    plt.ylabel("Loss")
    plt.title("MobileNetV2 V2 Loss")
    plt.legend()
    plt.tight_layout()
    plt.savefig(OUTPUT_DIR / "training_loss.png", dpi=180)
    plt.close()

    keys = sorted(history)
    with (OUTPUT_DIR / "training_history.csv").open(
        "w", newline="", encoding="utf-8"
    ) as file:
        writer = csv.writer(file)
        writer.writerow(["epoch", *keys])
        for index in range(len(epochs)):
            writer.writerow(
                [
                    index + 1,
                    *[
                        history[key][index] if index < len(history[key]) else ""
                        for key in keys
                    ],
                ]
            )


def evaluate_accuracy(model: tf.keras.Model, val_ds: tf.data.Dataset) -> float:
    metrics = model.evaluate(val_ds, verbose=0, return_dict=True)
    return float(metrics["accuracy"])


def select_best_checkpoint(
    val_ds: tf.data.Dataset,
) -> tuple[tf.keras.Model, str, dict[str, float]]:
    candidates = {
        "head": OUTPUT_DIR / "best_head.keras",
        "fine_tuned": OUTPUT_DIR / "best_fine_tuned.keras",
    }

    scores: dict[str, float] = {}
    loaded_models: dict[str, tf.keras.Model] = {}

    for name, path in candidates.items():
        if not path.exists():
            continue

        candidate = tf.keras.models.load_model(path)
        score = evaluate_accuracy(candidate, val_ds)
        scores[name] = score
        loaded_models[name] = candidate
        print(f"Checkpoint {name}: validation accuracy = {score:.4f}")

    if not scores:
        raise FileNotFoundError("No saved checkpoint was found.")

    selected_name = max(scores, key=scores.get)
    print(f"Selected checkpoint: {selected_name}")
    return loaded_models[selected_name], selected_name, scores


def evaluate_model(
    model: tf.keras.Model,
    val_ds: tf.data.Dataset,
) -> dict:
    y_true: list[int] = []
    y_pred: list[int] = []
    probabilities: list[list[float]] = []

    for images, labels in val_ds:
        batch_probabilities = model.predict(images, verbose=0)
        batch_predictions = np.argmax(batch_probabilities, axis=1)

        y_true.extend(labels.numpy().astype(int).tolist())
        y_pred.extend(batch_predictions.astype(int).tolist())
        probabilities.extend(batch_probabilities.astype(float).tolist())

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

    display = ConfusionMatrixDisplay(
        confusion_matrix=matrix,
        display_labels=CLASS_NAMES,
    )
    display.plot(cmap="Blues", values_format="d")
    plt.title("Validation Confusion Matrix")
    plt.tight_layout()
    plt.savefig(OUTPUT_DIR / "validation_confusion_matrix.png", dpi=180)
    plt.close()

    with (OUTPUT_DIR / "validation_predictions.csv").open(
        "w", newline="", encoding="utf-8"
    ) as file:
        writer = csv.writer(file)
        writer.writerow(
            ["actual", "predicted", *[f"p_{name}" for name in CLASS_NAMES]]
        )
        for actual, predicted, probs in zip(y_true, y_pred, probabilities):
            writer.writerow(
                [
                    CLASS_NAMES[actual],
                    CLASS_NAMES[predicted],
                    *probs,
                ]
            )

    result = {
        "class_names": CLASS_NAMES,
        "classification_report": report,
        "confusion_matrix": matrix.tolist(),
    }
    with (OUTPUT_DIR / "validation_metrics.json").open(
        "w", encoding="utf-8"
    ) as file:
        json.dump(result, file, indent=2)

    return result


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    set_reproducible_seed(SEED)

    counts = validate_dataset_structure(DATASET_DIR)
    print("Dataset counts:")
    for class_name in CLASS_NAMES:
        print(f"  {class_name}: {counts[class_name]}")

    with (OUTPUT_DIR / "dataset_counts.json").open("w", encoding="utf-8") as file:
        json.dump(counts, file, indent=2)

    train_ds, val_ds = create_datasets(DATASET_DIR)
    model, base_model = build_model()

    print("\nStage 1: training the new classification head...")
    compile_model(model, HEAD_LEARNING_RATE)
    head_history = model.fit(
        train_ds,
        validation_data=val_ds,
        epochs=HEAD_EPOCHS,
        callbacks=callbacks_for("head"),
    )

    print("\nStage 2: fine-tuning the final MobileNetV2 layers...")
    base_model.trainable = True
    freeze_until = max(0, len(base_model.layers) - FINE_TUNE_LAST_LAYERS)

    for index, layer in enumerate(base_model.layers):
        if index < freeze_until or isinstance(layer, tf.keras.layers.BatchNormalization):
            layer.trainable = False
        else:
            layer.trainable = True

    compile_model(model, FINE_TUNE_LEARNING_RATE)
    fine_tune_history = model.fit(
        train_ds,
        validation_data=val_ds,
        epochs=FINE_TUNE_EPOCHS,
        callbacks=callbacks_for("fine_tuned"),
    )

    history = merge_histories(head_history, fine_tune_history)
    save_training_curves(history)

    # Select the best validation-accuracy checkpoint across both stages.
    model, selected_checkpoint, checkpoint_scores = select_best_checkpoint(val_ds)

    final_model_path = OUTPUT_DIR / "emotioncam_mobilenetv2_float.keras"
    model.save(final_model_path)

    with (OUTPUT_DIR / "class_names.json").open("w", encoding="utf-8") as file:
        json.dump(CLASS_NAMES, file, indent=2)

    with (OUTPUT_DIR / "model_metadata.json").open("w", encoding="utf-8") as file:
        json.dump(
            {
                "architecture": "MobileNetV2",
                "version": "V2",
                "pretrained_weights": "ImageNet",
                "alpha": ALPHA,
                "input_shape": [IMAGE_HEIGHT, IMAGE_WIDTH, CHANNELS],
                "center_crop_size": CENTER_CROP_SIZE,
                "input_dtype_before_quantization": "float32",
                "input_range_before_quantization": [0, 255],
                "classes": CLASS_NAMES,
                "parameters": int(model.count_params()),
                "selected_checkpoint": selected_checkpoint,
                "checkpoint_validation_accuracy": checkpoint_scores,
            },
            file,
            indent=2,
        )

    results = evaluate_model(model, val_ds)
    accuracy = float(results["classification_report"]["accuracy"])

    print("\nTraining complete.")
    print(f"Best float validation accuracy: {accuracy:.4f}")
    print(f"Selected checkpoint: {selected_checkpoint}")
    print(f"Saved model: {final_model_path}")
    print("Next command: python convert_int8_tflite.py")


if __name__ == "__main__":
    main()
