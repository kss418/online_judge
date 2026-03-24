#!/usr/bin/env bash

declare -ag flow_test_temp_files=()
declare -ag flow_test_temp_dirs=()

init_flow_test(){
    flow_test_temp_files=()
    flow_test_temp_dirs=()
}

register_temp_file(){
    local temp_file_var_name="$1"
    local temp_file_path=""

    if [[ -z "${temp_file_var_name}" ]]; then
        echo "missing temp_file_var_name" >&2
        return 1
    fi

    temp_file_path="$(mktemp)"
    printf -v "${temp_file_var_name}" '%s' "${temp_file_path}"
    flow_test_temp_files+=("${temp_file_path}")
}

register_temp_dir(){
    local temp_dir_var_name="$1"
    local temp_dir_path=""

    if [[ -z "${temp_dir_var_name}" ]]; then
        echo "missing temp_dir_var_name" >&2
        return 1
    fi

    temp_dir_path="$(mktemp -d)"
    printf -v "${temp_dir_var_name}" '%s' "${temp_dir_path}"
    flow_test_temp_dirs+=("${temp_dir_path}")
}

cleanup_flow_test_temp_resources(){
    if (( ${#flow_test_temp_files[@]} > 0 )); then
        rm -f "${flow_test_temp_files[@]}"
    fi

    if (( ${#flow_test_temp_dirs[@]} > 0 )); then
        rm -rf "${flow_test_temp_dirs[@]}"
    fi
}

finish_flow_test(){
    local cleanup_callback=""
    local finish_status=0

    while (( $# > 0 )); do
        cleanup_callback="$1"
        shift

        if [[ -z "${cleanup_callback}" ]]; then
            continue
        fi

        if ! declare -F "${cleanup_callback}" >/dev/null 2>&1; then
            echo "unknown cleanup callback: ${cleanup_callback}" >&2
            finish_status=1
            continue
        fi

        if ! "${cleanup_callback}"; then
            finish_status=1
        fi
    done

    cleanup_flow_test_temp_resources
    return "${finish_status}"
}

print_success_log(){
    local log_message="$1"

    if [[ -z "${log_message}" ]]; then
        echo "missing log_message" >&2
        return 1
    fi

    if [[ -z "${test_log_temp_file:-}" ]]; then
        echo "missing test_log_temp_file" >&2
        return 1
    fi

    printf '%s\n' "${log_message}"

    if [[ "$(type -t append_log_line || true)" == "function" ]]; then
        append_log_line "${test_log_temp_file}" "${log_message}"
        return 0
    fi

    printf '%s\n' "${log_message}" >> "${test_log_temp_file}"
}
