#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
backend_dir="$(cd "${script_dir}/.." && pwd)"

build_dir="${BACKEND_BUILD_DIR:-${backend_dir}/build}"
build_type="${BACKEND_BUILD_TYPE:-Release}"
build_parallel="${BACKEND_BUILD_PARALLEL:-2}"
cmake_generator="${BACKEND_CMAKE_GENERATOR:-Unix Makefiles}"

if [[ ! "${build_parallel}" =~ ^[1-9][0-9]*$ ]]; then
    echo "error: BACKEND_BUILD_PARALLEL must be a positive integer: ${build_parallel}" >&2
    exit 1
fi

targets=("$@")
if (( ${#targets[@]} == 0 )); then
    targets=(http_server judge_server)
fi

cmake -S "${backend_dir}" -B "${build_dir}" -G "${cmake_generator}" \
    -DCMAKE_BUILD_TYPE="${build_type}" \
    -DCMAKE_TOOLCHAIN_FILE="${backend_dir}/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_INSTALLED_DIR="${backend_dir}/vcpkg_installed"

cmake --build "${build_dir}" --parallel "${build_parallel}" --target "${targets[@]}"
