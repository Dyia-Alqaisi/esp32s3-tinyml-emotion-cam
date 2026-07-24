from __future__ import annotations

from pathlib import Path
from typing import Iterable

import tensorflow as tf

from config import (
    BATCH_SIZE,
    CENTER_CROP_SIZE,
    CLASS_NAMES,
    DATASET_DIR,
    IMAGE_HEIGHT,
    IMAGE_WIDTH,
    RAW_IMAGE_HEIGHT,
    RAW_IMAGE_WIDTH,
    SEED,
    VALIDATION_SPLIT,
)


def validate_dataset_structure(dataset_dir: Path = DATASET_DIR) -> dict[str, int]:
    if not dataset_dir.exists():
        raise FileNotFoundError(f"Dataset directory does not exist: {dataset_dir}")

    valid_extensions = {".jpg", ".jpeg", ".png", ".bmp"}
    counts: dict[str, int] = {}

    for class_name in CLASS_NAMES:
        class_dir = dataset_dir / class_name
        if not class_dir.is_dir():
            raise FileNotFoundError(f"Missing required class directory: {class_dir}")

        files = [
            path for path in class_dir.iterdir()
            if path.is_file() and path.suffix.lower() in valid_extensions
        ]
        if not files:
            raise ValueError(f"No supported images found in: {class_dir}")
        counts[class_name] = len(files)

    return counts


def _augmentation_pipeline() -> tf.keras.Sequential:
    # Realistic training-only changes. No vertical flip and no extreme rotation.
    return tf.keras.Sequential(
        [
            tf.keras.layers.RandomFlip("horizontal"),
            tf.keras.layers.RandomTranslation(
                height_factor=0.04,
                width_factor=0.04,
                fill_mode="reflect",
            ),
            tf.keras.layers.RandomRotation(0.035, fill_mode="reflect"),
            tf.keras.layers.RandomZoom(
                height_factor=(-0.08, 0.08),
                width_factor=(-0.08, 0.08),
                fill_mode="reflect",
            ),
            tf.keras.layers.RandomContrast(0.12),
        ],
        name="training_only_augmentation",
    )


def _center_crop_and_resize(images: tf.Tensor) -> tf.Tensor:
    images = tf.image.resize_with_crop_or_pad(
        images,
        target_height=CENTER_CROP_SIZE,
        target_width=CENTER_CROP_SIZE,
    )
    images = tf.image.resize(
        images,
        size=(IMAGE_HEIGHT, IMAGE_WIDTH),
        method="bilinear",
        antialias=True,
    )
    return tf.cast(images, tf.float32)


def _base_directory_dataset(
    dataset_dir: Path,
    subset: str,
    labels: str | None = "inferred",
    label_mode: str | None = "int",
    batch_size: int = BATCH_SIZE,
) -> tf.data.Dataset:
    kwargs = dict(
        directory=str(dataset_dir),
        labels=labels,
        color_mode="rgb",
        batch_size=batch_size,
        image_size=(RAW_IMAGE_HEIGHT, RAW_IMAGE_WIDTH),
        shuffle=True,
        seed=SEED,
        crop_to_aspect_ratio=False,
    )

    if labels == "inferred":
        kwargs["label_mode"] = label_mode
        kwargs["class_names"] = CLASS_NAMES
        kwargs["validation_split"] = VALIDATION_SPLIT
        kwargs["subset"] = subset

    return tf.keras.utils.image_dataset_from_directory(**kwargs)


def create_datasets(
    dataset_dir: Path = DATASET_DIR,
) -> tuple[tf.data.Dataset, tf.data.Dataset]:
    train_ds = _base_directory_dataset(dataset_dir, subset="training")
    val_ds = _base_directory_dataset(dataset_dir, subset="validation")

    augmentation = _augmentation_pipeline()

    def preprocess_train(images: tf.Tensor, labels: tf.Tensor):
        images = _center_crop_and_resize(images)
        images = augmentation(images, training=True)
        return images, labels

    def preprocess_val(images: tf.Tensor, labels: tf.Tensor):
        return _center_crop_and_resize(images), labels

    autotune = tf.data.AUTOTUNE
    train_ds = train_ds.map(preprocess_train, num_parallel_calls=autotune)
    val_ds = val_ds.map(preprocess_val, num_parallel_calls=autotune)

    return train_ds.prefetch(autotune), val_ds.prefetch(autotune)


def create_evaluation_dataset(
    dataset_dir: Path = DATASET_DIR,
) -> tf.data.Dataset:
    val_ds = _base_directory_dataset(dataset_dir, subset="validation")

    def preprocess(images: tf.Tensor, labels: tf.Tensor):
        return _center_crop_and_resize(images), labels

    return val_ds.map(
        preprocess,
        num_parallel_calls=tf.data.AUTOTUNE,
    ).prefetch(tf.data.AUTOTUNE)


def _load_single_image(path: tf.Tensor) -> tf.Tensor:
    encoded = tf.io.read_file(path)
    image = tf.io.decode_image(encoded, channels=3, expand_animations=False)
    image.set_shape([None, None, 3])
    image = tf.image.resize_with_crop_or_pad(image, target_height=CENTER_CROP_SIZE, target_width=CENTER_CROP_SIZE)
    image = tf.image.resize(image, size=(IMAGE_HEIGHT, IMAGE_WIDTH), method="bilinear", antialias=True)
    return tf.cast(image, tf.float32)


def balanced_representative_paths(dataset_dir: Path = DATASET_DIR) -> list[Path]:
    valid_extensions = {".jpg", ".jpeg", ".png", ".bmp"}
    per_class: dict[str, list[Path]] = {}
    for class_name in CLASS_NAMES:
        class_dir = dataset_dir / class_name
        paths = sorted(path for path in class_dir.iterdir() if path.is_file() and path.suffix.lower() in valid_extensions)
        if not paths:
            raise ValueError(f"No calibration images found in {class_dir}")
        per_class[class_name] = paths

    ordered: list[Path] = []
    max_count = max(len(paths) for paths in per_class.values())
    for index in range(max_count):
        for class_name in CLASS_NAMES:
            paths = per_class[class_name]
            if index < len(paths):
                ordered.append(paths[index])
    return ordered


def create_representative_dataset(dataset_dir: Path = DATASET_DIR, max_samples: int = 240) -> Iterable[list[tf.Tensor]]:
    paths = balanced_representative_paths(dataset_dir)[:max_samples]
    for path in paths:
        image = _load_single_image(tf.constant(str(path)))
        yield [tf.expand_dims(image, axis=0)]
