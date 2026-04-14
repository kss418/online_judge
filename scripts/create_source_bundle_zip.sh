#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"

archive_paths=(
    "backend/main"
    "backend/scripts"
    "backend/src"
    "backend/include"
    "backend/docs"
    "frontend/src"
    "scripts"
)

usage(){
    cat <<'EOF'
usage: create_source_bundle_zip.sh [output.zip]

Creates a zip archive containing:
  - backend/main
  - backend/scripts
  - backend/src
  - backend/include
  - backend/docs
  - frontend/src
  - scripts

If no output path is provided, the archive is created in the project root with
the name online_judge_source_bundle_YYYYMMDD_HHMMSS.zip.
EOF
}

if (( $# > 1 )); then
    usage >&2
    exit 1
fi

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if ! command -v zip >/dev/null 2>&1; then
    echo "error: zip command is required but was not found" >&2
    exit 1
fi

timestamp="$(date +%Y%m%d_%H%M%S)"
default_output="${project_root}/online_judge_source_bundle_${timestamp}.zip"

if (( $# == 1 )); then
    if [[ "$1" = /* ]]; then
        output_path="$1"
    else
        output_path="$(pwd)/$1"
    fi
else
    output_path="${default_output}"
fi

output_dir="$(dirname "${output_path}")"
mkdir -p "${output_dir}"

for archive_path in "${archive_paths[@]}"; do
    if [[ ! -d "${project_root}/${archive_path}" ]]; then
        echo "error: missing directory: ${project_root}/${archive_path}" >&2
        exit 1
    fi
done

temp_dir="$(mktemp -d /tmp/online_judge_source_bundle.XXXXXX)"
temp_archive="${temp_dir}/source_bundle.zip"
cleanup(){
    rm -rf "${temp_dir}"
}
trap cleanup EXIT

(
    cd "${project_root}"
    zip -r -q "${temp_archive}" "${archive_paths[@]}"
)

mv "${temp_archive}" "${output_path}"
trap - EXIT

echo "created archive: ${output_path}"
