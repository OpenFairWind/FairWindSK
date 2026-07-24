#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="${1:-${repo_dir}/build-manual}"
executable="${build_dir}/FairWindSK"

if [[ ! -x "${executable}" && -x "${build_dir}/FairWindSK.app/Contents/MacOS/FairWindSK" ]]; then
    executable="${build_dir}/FairWindSK.app/Contents/MacOS/FairWindSK"
fi

if [[ ! -x "${executable}" ]]; then
    echo "FairWindSK executable not found at ${executable}." >&2
    echo "Configure and build the project first, or pass the build directory." >&2
    exit 1
fi

capture_language() {
    local locale="$1"
    local figures_dir="$2"
    mkdir -p "${figures_dir}"
    FAIRWINDSK_MANUAL_SCREENSHOT_DIR="${figures_dir}" LANG="${locale}.UTF-8" "${executable}"
}

capture_language en_US "${repo_dir}/docs/manual/english/figures"
capture_language fr_FR "${repo_dir}/docs/manual/french/figures"
capture_language es_ES "${repo_dir}/docs/manual/spanish/figures"
capture_language it_IT "${repo_dir}/docs/manual/italian/figures"
