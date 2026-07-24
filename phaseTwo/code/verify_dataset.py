from pathlib import Path

from PIL import Image, UnidentifiedImageError

from config import CLASS_NAMES, DATASET_DIR, SUPPORTED_EXTENSIONS


def main() -> None:
    total = 0
    bad_files: list[str] = []

    for class_name in CLASS_NAMES:
        class_dir = DATASET_DIR / class_name
        count = 0

        if not class_dir.exists():
            print(f"MISSING: {class_dir}")
            continue

        for path in sorted(class_dir.iterdir()):
            if not path.is_file() or path.suffix.lower() not in SUPPORTED_EXTENSIONS:
                continue

            count += 1
            total += 1

            try:
                with Image.open(path) as image:
                    image.verify()
            except (OSError, ValueError, UnidentifiedImageError) as exc:
                bad_files.append(f"{path}: {exc}")

        print(f"{class_name}: {count}")

    print(f"Total: {total}")

    if bad_files:
        print("\nUnreadable files:")
        for item in bad_files:
            print(f"  {item}")
        raise SystemExit(1)

    print("All image files can be decoded.")


if __name__ == "__main__":
    main()
