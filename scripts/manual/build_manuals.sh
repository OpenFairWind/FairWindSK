#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
capture=true
build_dir="${repo_dir}/build-manual"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-screenshots)
            capture=false
            shift
            ;;
        --build-dir)
            build_dir="$2"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

if [[ "${capture}" == true ]]; then
    "${repo_dir}/scripts/manual/capture_screenshots.sh" "${build_dir}"
fi

build_one() {
    local manual_dir="$1"
    local source_file="$2"
    local output_file="$3"
    local version
    local git_hash
    local git_branch
    local git_date
    local build_date
    local build_year

    version="$(tr -d '[:space:]' < "${repo_dir}/VERSION.txt")"
    git_hash="$(git -C "${repo_dir}" rev-parse --short HEAD 2>/dev/null || printf unknown)"
    git_branch="$(git -C "${repo_dir}" branch --show-current 2>/dev/null || printf unknown)"
    git_date="$(git -C "${repo_dir}" log -1 --format=%cs 2>/dev/null || printf unknown)"
    build_date="$(date +%Y-%m-%d)"
    build_year="$(date +%Y)"

    printf '%s\n' \
        '% AUTO-GENERATED - DO NOT EDIT' \
        "\\newcommand{\\fwVersion}{${version}}" \
        '\newcommand{\fwVersionMinor}{0}' \
        '\newcommand{\fwVersionPatch}{0}' \
        "\\newcommand{\\fwGitHash}{${git_hash}}" \
        '\newcommand{\fwGitCount}{0}' \
        "\\newcommand{\\fwGitBranch}{${git_branch}}" \
        "\\newcommand{\\fwGitDate}{${git_date}}" \
        "\\newcommand{\\fwBuildDate}{${build_date}}" \
        "\\newcommand{\\fwBuildYear}{${build_year}}" \
        '\newcommand{\fwEdition}{2.2}' > "${manual_dir}/version.tex"

    for pass in 1 2 3; do
        (cd "${manual_dir}" && xelatex -halt-on-error -interaction=nonstopmode "${source_file}")
    done
    pdfinfo "${manual_dir}/${output_file}" | grep -E '^(Pages|Page size|PDF version):'
}

build_one "${repo_dir}/docs/manual/english" fairwindsk-manual.tex fairwindsk-manual.pdf
build_one "${repo_dir}/docs/manual/french" fairwindsk-manuel.tex fairwindsk-manuel.pdf
build_one "${repo_dir}/docs/manual/spanish" fairwindsk-manual.tex fairwindsk-manual.pdf
build_one "${repo_dir}/docs/manual/italian" fairwindsk-manuale.tex fairwindsk-manuale.pdf
