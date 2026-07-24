from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageFile, UnidentifiedImageError

from config import CLASS_NAMES, DATASET_DIR, SUPPORTED_EXTENSIONS

ImageFile.LOAD_TRUNCATED_IMAGES = False


def inspect_image(path: Path) -> list[str]:
    problems: list[str] = []
    raw = path.read_bytes()

    if path.suffix.lower() in {".jpg", ".jpeg"}:
        if not raw.startswith(b"\xff\xd8"):
            problems.append("missing JPEG start marker")
        if not raw.rstrip().endswith(b"\xff\xd9"):
            problems.append("missing JPEG end marker")

    try:
        with Image.open(path) as image:
            image.load()
            if image.width <= 0 or image.height <= 0:
                problems.append("invalid dimensions")
    except (OSError, ValueError, UnidentifiedImageError) as exc:
        problems.append(str(exc))

    return problems


def main() -> None:
    inspected = 0
    bad: list[tuple[Path, list[str]]] = []

    for class_name in CLASS_NAMES:
        class_dir = DATASET_DIR / class_name
        for path in sorted(class_dir.iterdir()):
            if not path.is_file() or path.suffix.lower() not in SUPPORTED_EXTENSIONS:
                continue

            inspected += 1
            problems = inspect_image(path)
            if problems:
                bad.append((path, problems))

    print(f"Inspected images: {inspected}")
    print(f"Problematic images: {len(bad)}")

    for path, problems in bad:
        print(f"- {path.relative_to(DATASET_DIR)}")
        for problem in problems:
            print(f"    {problem}")

    if bad:
        print(
            "\nRecapture or remove these files before training. "
            "Do not merely suppress the warning."
        )
        raise SystemExit(1)

    print("No JPEG marker or full-decoding problems were found.")


if __name__ == "__main__":
    main()
