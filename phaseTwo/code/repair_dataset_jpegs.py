from __future__ import annotations

import shutil
from pathlib import Path
from PIL import Image, ImageFile, UnidentifiedImageError
from config import CLASS_NAMES, DATASET_DIR, RAW_DATASET_DIR, SUPPORTED_EXTENSIONS

ImageFile.LOAD_TRUNCATED_IMAGES = True


def reencode_image(source: Path, destination: Path) -> tuple[bool, str]:
    try:
        with Image.open(source) as image:
            image = image.convert("RGB")
            destination.parent.mkdir(parents=True, exist_ok=True)
            image.save(destination.with_suffix(".jpg"), format="JPEG", quality=95, subsampling=0, optimize=False)
        return True, "re-encoded"
    except (OSError, ValueError, UnidentifiedImageError) as exc:
        return False, str(exc)


def main() -> None:
    if not RAW_DATASET_DIR.exists():
        raise FileNotFoundError(f"Raw dataset folder not found: {RAW_DATASET_DIR}")
    if DATASET_DIR.exists():
        shutil.rmtree(DATASET_DIR)
    DATASET_DIR.mkdir(parents=True)

    converted = 0
    failed: list[tuple[Path, str]] = []
    for class_name in CLASS_NAMES:
        source_dir = RAW_DATASET_DIR / class_name
        destination_dir = DATASET_DIR / class_name
        destination_dir.mkdir(parents=True, exist_ok=True)
        if not source_dir.is_dir():
            raise FileNotFoundError(f"Missing class folder: {source_dir}")
        for source in sorted(source_dir.iterdir()):
            if not source.is_file() or source.suffix.lower() not in SUPPORTED_EXTENSIONS:
                continue
            destination = destination_dir / source.name
            ok, message = reencode_image(source, destination)
            if ok:
                converted += 1
            else:
                failed.append((source, message))

    print(f"Re-encoded images: {converted}")
    print(f"Failed images: {len(failed)}")
    print(f"Repaired dataset: {DATASET_DIR}")
    if failed:
        for path, message in failed:
            print(f"- {path}: {message}")
        raise SystemExit(1)
    print("The original dataset was not modified. Training/conversion scripts now use dataset_repaired.")


if __name__ == "__main__":
    main()
