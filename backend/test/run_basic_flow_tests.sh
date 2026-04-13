#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
backend_dir="$(cd "${script_dir}/.." && pwd)"
build_dir="${BACKEND_BUILD_DIR:-${backend_dir}/build}"
skip_build="${BASIC_FLOW_TESTS_SKIP_BUILD:-0}"
clean_build="${BASIC_FLOW_TESTS_CLEAN_BUILD:-1}"

default_tests=(
    postgres_lib_flow.sh
    system_flow.sh
    auth_flow.sh
    user_flow.sh
    problem_get_flow.sh
    problem_list_flow.sh
    problem_lifecycle_flow.sh
    problem_statement_flow.sh
    problem_sample_flow.sh
    problem_testcase_flow.sh
    submission_flow.sh
    http_error_contract_flow.sh
    user_problem_schema_flow.sh
)

tests=("$@")
if (( ${#tests[@]} == 0 )); then
    tests=("${default_tests[@]}")
fi

if [[ "${skip_build}" != "1" ]]; then
    build_args=(
        --build "${build_dir}"
        --target http_server
        --target judge_server
    )
    if [[ "${clean_build}" == "1" ]]; then
        build_args=(--build "${build_dir}" --clean-first "${build_args[@]:2}")
    fi

    cmake "${build_args[@]}"
fi

for test_script in "${tests[@]}"; do
    test_path="${script_dir}/${test_script}"
    if [[ ! -f "${test_path}" ]]; then
        echo "missing test script: ${test_path}" >&2
        exit 1
    fi

    printf '\n==> Running %s\n' "${test_script}"
    bash "${test_path}"
done

printf '\nBasic backend flow tests passed.\n'
