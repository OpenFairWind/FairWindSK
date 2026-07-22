#!/usr/bin/env python3
"""Create the deterministic release manifest consumed by Sigstore signing."""

import argparse
import hashlib
import json
from pathlib import Path


def sha256(path: Path) -> str:
    """Hash an artifact in chunks so large installers do not fill memory."""
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    """Validate inputs and write stable, reviewable JSON plus checksum text."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    generated_names = {
        args.output.name,
        "SHA256SUMS",
        "release-manifest.pem",
        "release-manifest.sig",
    }
    files = sorted(
        path
        for path in args.artifacts.iterdir()
        if path.is_file() and path.name not in generated_names
    )
    if not files:
        parser.error("no release artifacts were found")

    required_platforms = (
        "linux-x86_64",
        "raspberry-pi-os-arm64",
        "windows-x86_64",
        "macos-arm64",
    )
    missing_platforms = [
        platform
        for platform in required_platforms
        if not any(platform in path.name for path in files)
    ]
    if missing_platforms:
        parser.error(f"missing required platform artifacts: {', '.join(missing_platforms)}")

    artifacts = [
        {"name": path.name, "sha256": sha256(path), "size": path.stat().st_size}
        for path in files
    ]
    manifest = {
        "artifacts": artifacts,
        "commit": args.commit,
        "schemaVersion": 1,
        "tag": args.tag,
        "version": args.version,
    }
    args.output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    checksums = args.output.with_name("SHA256SUMS")
    checksums.write_text("".join(f"{item['sha256']}  {item['name']}\n" for item in artifacts))


if __name__ == "__main__":
    main()
