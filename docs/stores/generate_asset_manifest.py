#!/usr/bin/env python3
"""Create an auditable JSON manifest for FairWindSK store raster assets."""

import hashlib
import json
from pathlib import Path

from PIL import Image


# Locate the store package without relying on the caller's working directory.
STORES = Path(__file__).resolve().parent
# Keep the generated inventory at the package root for easy review.
DESTINATION = STORES / "asset-manifest.json"


def sha256(path: Path) -> str:
    """Return the SHA-256 digest of one complete file."""
    # Create an isolated digest for this asset.
    digest = hashlib.sha256()
    # Open the binary file without loading the entire raster into memory.
    with path.open("rb") as source:
        # Read fixed-size chunks for predictable memory use.
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            # Feed each chunk to the cryptographic digest.
            digest.update(chunk)
    # Return the portable lowercase hexadecimal representation.
    return digest.hexdigest()


def main() -> None:
    """Inventory every final PNG while excluding the editable feature source."""
    # Prepare a stable list for deterministic diffs.
    assets: list[dict[str, object]] = []
    # Walk every store PNG in lexical order.
    for path in sorted(STORES.rglob("*.png")):
        # Exclude the generated feature background because it is an editable source.
        if path.name == "feature-graphic-background.png":
            # Continue with upload-ready artwork.
            continue
        # Decode the PNG to validate and record its raster properties.
        with Image.open(path) as image:
            # Add one self-contained manifest record.
            assets.append(
                {
                    # Store portable repository-relative path separators.
                    "path": path.relative_to(STORES).as_posix(),
                    # Record exact width in pixels.
                    "width": image.width,
                    # Record exact height in pixels.
                    "height": image.height,
                    # Record the color mode used by store validation.
                    "mode": image.mode,
                    # Record file size for upload and repository auditing.
                    "bytes": path.stat().st_size,
                    # Detect any accidental binary replacement.
                    "sha256": sha256(path),
                }
            )
    # Wrap records with a machine-readable schema identifier and count.
    manifest = {
        # Version the local schema independently of app releases.
        "schema": 1,
        # State exactly what this manifest inventories.
        "description": "Upload-ready FairWindSK App Store and Google Play raster assets.",
        # Make accidental omissions visible.
        "asset_count": len(assets),
        # Preserve the ordered raster records.
        "assets": assets,
    }
    # Write UTF-8 JSON with stable indentation and a final newline.
    DESTINATION.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


# Generate the manifest only when invoked as a script.
if __name__ == "__main__":
    # Enter the deterministic inventory process.
    main()
