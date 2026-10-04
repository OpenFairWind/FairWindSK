#!/usr/bin/env python3
"""Repository-owned CI checks that need project-specific policy."""

import argparse
import json
import os
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE_SUFFIXES = {".cpp", ".hpp"}
VISIBLE_CALL = re.compile(r"\b(?:setText|setTitle|setToolTip|setPlaceholderText|setStatusTip)\s*\(\s*(?:QStringLiteral\s*\()?\s*\"" )


def translation_check(maximum_unfinished: int) -> int:
    # Parse every catalog so malformed XML fails before Qt's resource compilation.
    catalogs = sorted((ROOT / "resources/i18n").glob("*.ts"))
    if not catalogs:
        print("No translation catalogs found", file=sys.stderr)
        return 1
    unfinished = 0
    empty_finished = []
    for catalog in catalogs:
        tree = ET.parse(catalog)
        for message in tree.findall(".//message"):
            translation = message.find("translation")
            if translation is None or translation.get("type") == "unfinished":
                unfinished += 1
            elif translation.get("type") not in {"vanished", "obsolete"} and not "".join(translation.itertext()).strip():
                empty_finished.append(str(catalog.relative_to(ROOT)))
    print(f"Translation debt: {unfinished} unfinished message(s); permitted baseline: {maximum_unfinished}")
    if empty_finished:
        print("Finished translations may not be empty: " + ", ".join(sorted(set(empty_finished))), file=sys.stderr)
        return 1
    if unfinished > maximum_unfinished:
        print("Translation completeness regressed; translate new messages or deliberately update the reviewed baseline.", file=sys.stderr)
        return 1
    return 0


def changed_lines() -> list[tuple[Path, int, str]]:
    # Limit the literal heuristic to added lines so historical localization debt stays visible but does not hide regressions.
    base = os.environ.get("FAIRWINDSK_DIFF_BASE", "").strip()
    command = ["git", "diff", "--unified=0"]
    if base and set(base) != {"0"}:
        command.append(base)
    command.extend(["--", "*.cpp", "*.hpp"])
    diff = subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True).stdout
    current = None
    line_number = 0
    result = []
    for line in diff.splitlines():
        if line.startswith("+++ b/"):
            current = ROOT / line[6:]
        elif line.startswith("@@"):
            match = re.search(r"\+(\d+)", line)
            line_number = int(match.group(1)) if match else 0
        elif line.startswith("+") and not line.startswith("+++"):
            if current is not None:
                result.append((current, line_number, line[1:]))
            line_number += 1
        elif not line.startswith("-"):
            line_number += 1
    return result


def missing_tr_check() -> int:
    failures = []
    for path, line_number, text in changed_lines():
        if VISIBLE_CALL.search(text) and "tr(" not in text and "translate(" not in text:
            failures.append(f"{path.relative_to(ROOT)}:{line_number}: visible string literal must use tr()")
    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    print("No added unwrapped Qt widget string literals found.")
    return 0


def changed_sources() -> list[Path]:
    # Select source files touched by the proposed change so legacy formatting debt cannot mask a new regression.
    base = os.environ.get("FAIRWINDSK_DIFF_BASE", "").strip()
    command = ["git", "diff", "--name-only", "--diff-filter=ACMR"]
    if base and set(base) != {"0"}:
        command.append(base)
    output = subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True).stdout
    return [ROOT / name for name in output.splitlines() if Path(name).suffix in SOURCE_SUFFIXES]


def clang_format_check() -> int:
    # Run the formatter in verification mode and never rewrite a contributor's files in CI.
    sources = changed_sources()
    if not sources:
        print("No changed C++ sources require clang-format validation.")
        return 0
    subprocess.run(["clang-format", "--dry-run", "--Werror", *map(str, sources)], cwd=ROOT, check=True)
    return 0


def clang_tidy_check(build_directory: str) -> int:
    # Use CMake's compilation database so clang-tidy sees the exact Qt include paths and compile definitions.
    config = ROOT / ".clang-tidy"
    if not config.is_file() or not config.read_text(encoding="utf-8").strip():
        print(".clang-tidy is missing or empty", file=sys.stderr)
        return 1
    sources = changed_sources()
    if not sources:
        print("No changed C++ sources require clang-tidy validation.")
        return 0
    subprocess.run(["clang-tidy", "-p", build_directory, *map(str, sources)], cwd=ROOT, check=True)
    return 0


def dependency_inventory_check() -> int:
    # Validate that every network-fetched CMake dependency is pinned rather than following a moving branch.
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    blocks = re.findall(r"(?:ExternalProject_Add|FetchContent_Declare)\s*\((.*?)\n\s*\)", cmake, re.S)
    failures = []
    for block in blocks:
        if "GIT_REPOSITORY" in block and not re.search(r"GIT_TAG\s+(?!main\b|master\b|HEAD\b)\S+", block):
            failures.append(block.split()[0])
    if failures:
        print("Unpinned CMake dependencies: " + ", ".join(failures), file=sys.stderr)
        return 1
    print(f"Validated {len(blocks)} pinned CMake dependency declaration(s).")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("check", choices=["translations", "missing-tr", "clang-format", "clang-tidy", "dependencies"])
    parser.add_argument("--maximum-unfinished", type=int, default=0)
    parser.add_argument("--build-directory", default="build")
    args = parser.parse_args()
    if args.check == "translations":
        return translation_check(args.maximum_unfinished)
    if args.check == "missing-tr":
        return missing_tr_check()
    if args.check == "clang-format":
        return clang_format_check()
    if args.check == "clang-tidy":
        return clang_tidy_check(args.build_directory)
    return dependency_inventory_check()


if __name__ == "__main__":
    sys.exit(main())
