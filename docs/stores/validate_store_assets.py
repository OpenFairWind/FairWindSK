#!/usr/bin/env python3
"""Validate FairWindSK store metadata limits and image specifications."""

import argparse
from pathlib import Path

from PIL import Image


# Resolve the publishing package independently of the current working directory.
STORES = Path(__file__).resolve().parent
# Collect validation failures so contributors see every issue in one run.
ERRORS: list[str] = []


def parse_arguments() -> argparse.Namespace:
    """Read the optional strict publishing check from the command line."""
    # Create a compact command-line interface suitable for contributors and CI.
    parser = argparse.ArgumentParser(description=__doc__)
    # Keep normal validation useful while publisher data is intentionally pending.
    parser.add_argument(
        "--strict",
        action="store_true",
        help="also fail when publisher-owned TODO values remain",
    )
    # Return the parsed immutable execution choice.
    return parser.parse_args()


def check_text(path: Path, limit: int, byte_limit: bool = False) -> None:
    """Verify that a required metadata file exists and respects its limit."""
    # Report a missing file as a publication blocker.
    if not path.is_file():
        # Preserve the relative path for actionable output.
        ERRORS.append(f"missing: {path.relative_to(STORES)}")
        # Skip length work for absent input.
        return
    # Remove the editor newline because stores do not count it as field content.
    value = path.read_text(encoding="utf-8").strip()
    # Select Apple's byte rule for keywords and character rules elsewhere.
    length = len(value.encode("utf-8")) if byte_limit else len(value)
    # Explain the exact overage when a field is too long.
    if length > limit:
        # Name the unit correctly for the store field.
        unit = "bytes" if byte_limit else "characters"
        # Add the failure without stopping later checks.
        ERRORS.append(f"{path.relative_to(STORES)}: {length}/{limit} {unit}")


def check_placeholders() -> None:
    """Reject unresolved publisher placeholders before a real submission."""
    # Search only source-controlled textual formats used by this package.
    for pattern in ("*.md", "*.txt", "*.yaml"):
        # Walk deterministically so strict CI output remains stable.
        for path in sorted(STORES.rglob(pattern)):
            # Decode every metadata document as UTF-8.
            value = path.read_text(encoding="utf-8")
            # Skip documents that only explain the placeholder convention.
            if path.name in {"README.md", "submission-checklist.md"}:
                # Continue with the remaining publication inputs.
                continue
            # Record each file containing unresolved account-holder data.
            if "TODO(PUBLISHER)" in value:
                # Report the exact relative path once.
                ERRORS.append(f"{path.relative_to(STORES)}: unresolved TODO(PUBLISHER)")


def check_image(path: Path, expected_size: tuple[int, int], count: int | None = None) -> None:
    """Verify one image or every PNG below a screenshot directory."""
    # Expand directories into deterministic filename order.
    paths = sorted(path.glob("*.png")) if path.is_dir() else [path]
    # Confirm screenshot-set cardinality where the package specifies it.
    if count is not None and len(paths) != count:
        # Describe the exact expected and actual counts.
        ERRORS.append(f"{path.relative_to(STORES)}: {len(paths)}/{count} images")
    # Inspect every upload file independently.
    for image_path in paths:
        # Detect missing required single-file assets.
        if not image_path.is_file():
            # Record a helpful missing path.
            ERRORS.append(f"missing: {image_path.relative_to(STORES)}")
            # Continue through the remaining checks.
            continue
        # Let Pillow decode the file and expose its dimensions and mode.
        with Image.open(image_path) as image:
            # Require the exact store dimensions.
            if image.size != expected_size:
                # Report actual dimensions for quick diagnosis.
                ERRORS.append(f"{image_path.relative_to(STORES)}: {image.size} != {expected_size}")
            # Prevent alpha channels in Apple screenshots and keep all assets simple.
            if image.mode != "RGB":
                # Report any palette, grayscale, or alpha mode.
                ERRORS.append(f"{image_path.relative_to(STORES)}: mode {image.mode} != RGB")


def main() -> None:
    """Run metadata and raster checks for both storefronts."""
    # Read whether this run represents a final submission gate.
    arguments = parse_arguments()
    # Validate each App Store locale against current App Store Connect limits.
    for locale in ("en-US", "it-IT"):
        # Resolve this locale once.
        metadata = STORES / "appstore" / "metadata" / locale
        # Check the 30-character product name.
        check_text(metadata / "name.txt", 30)
        # Check the 30-character subtitle.
        check_text(metadata / "subtitle.txt", 30)
        # Check the 170-character promotional field.
        check_text(metadata / "promotional_text.txt", 170)
        # Check Apple's 100-byte keyword field.
        check_text(metadata / "keywords.txt", 100, byte_limit=True)
        # Check the full plain-text product description.
        check_text(metadata / "description.txt", 4000)
        # Check the version release notes.
        check_text(metadata / "release_notes.txt", 4000)
        # Validate the selected 6.9-inch landscape screenshot family.
        check_image(STORES / "appstore" / "screenshots" / locale / "iphone-6.9-landscape", (2796, 1290), count=5)
        # Validate the selected 13-inch landscape screenshot family.
        check_image(STORES / "appstore" / "screenshots" / locale / "ipad-13-landscape", (2752, 2064), count=5)
    # Validate the App Store icon copied from the shipping catalog.
    check_image(STORES / "appstore" / "app-icon-1024.png", (1024, 1024))
    # Validate each Google Play locale against current Play Console limits.
    for locale in ("en-US", "it-IT"):
        # Resolve the locale metadata directory.
        metadata = STORES / "playstore" / "metadata" / locale
        # Check the 30-character Play title.
        check_text(metadata / "title.txt", 30)
        # Check the 80-character Play short description.
        check_text(metadata / "short_description.txt", 80)
        # Check the 4000-character Play full description.
        check_text(metadata / "full_description.txt", 4000)
        # Check Play release notes' 500-character locale limit.
        check_text(metadata / "release_notes.txt", 500)
        # Validate the landscape phone screenshot set.
        check_image(STORES / "playstore" / "screenshots" / locale / "phone", (1920, 1080), count=5)
        # Validate the landscape tablet screenshot set.
        check_image(STORES / "playstore" / "screenshots" / locale / "tablet", (2560, 1600), count=5)
    # Validate the required Google Play high-resolution icon.
    check_image(STORES / "playstore" / "graphics" / "app-icon-512.png", (512, 512))
    # Validate the required Google Play feature graphic.
    check_image(STORES / "playstore" / "graphics" / "feature-graphic-1024x500.png", (1024, 500))
    # Require publisher-owned fields only at the final submission gate.
    if arguments.strict:
        # Scan every store worksheet for unresolved decisions.
        check_placeholders()
    # Fail loudly after reporting every discovered problem.
    if ERRORS:
        # Print one stable line per issue for CI logs.
        raise SystemExit("Store asset validation failed:\n- " + "\n- ".join(ERRORS))
    # Provide a concise successful result for contributors and CI.
    print("Store asset validation passed.")


# Invoke validation only for direct script execution.
if __name__ == "__main__":
    # Enter the complete validator.
    main()
