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
    local language="$1"
    local figures_dir="$2"
    mkdir -p "${figures_dir}"
    if [[ "${language}" == it ]]; then
        FAIRWINDSK_MANUAL_SCREENSHOT_DIR="${figures_dir}" LANG=it_IT.UTF-8 "${executable}"
    else
        FAIRWINDSK_MANUAL_SCREENSHOT_DIR="${figures_dir}" LANG=en_US.UTF-8 "${executable}"
    fi

}

capture_language en "${repo_dir}/docs/manual/english/figures"
capture_language it "${repo_dir}/docs/manual/italian/figures"
