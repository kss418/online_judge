#!/usr/bin/env bash

test_util_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
test_util_project_root="$(cd "${test_util_dir}/.." && pwd)"

require_command(){
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "missing command: $1" >&2
        return 1
    fi
}

create_log_file(){
    local log_name="$1"
    local log_base_name=""
    local log_extension=""
    local timestamp=""
    local log_file_path=""

    if [[ -z "${log_name}" ]]; then
        echo "missing log_name" >&2
        return 1
    fi

    mkdir -p "${test_util_project_root}/log"

    timestamp="$(date +%y%m%d_%H%M%S)"

    if [[ "${log_name}" == *.* ]]; then
        log_base_name="${log_name%.*}"
        log_extension=".${log_name##*.}"
    else
        log_base_name="${log_name}"
        log_extension=""
    fi

    log_file_path="${test_util_project_root}/log/${log_base_name}_${timestamp}${log_extension}"
    : > "${log_file_path}"

    printf '%s\n' "${log_file_path}"
}

print_log_file_created(){
    local log_file_path="$1"

    if [[ -z "${log_file_path}" ]]; then
        echo "missing log_file_path" >&2
        return 1
    fi

    printf 'created log file: %s\n' "${log_file_path}"
}

append_log_line(){
    local log_file_path="$1"
    local log_message="$2"

    if [[ -z "${log_file_path}" ]]; then
        echo "missing log_file_path" >&2
        return 1
    fi

    printf '%s\n' "${log_message}" >> "${log_file_path}"
}

make_test_login_id(){
    local prefix="${1:-u}"
    local suffix="${2:-}"
    local unique_part=""

    prefix="${prefix:0:2}"
    if [[ -z "${prefix}" ]]; then
        prefix="u"
    fi

    unique_part="$(printf '%05d%05d' "$$" "$RANDOM")"
    unique_part="${unique_part:0:8}"

    printf '%s%s%s\n' "${prefix}" "${unique_part}" "${suffix}"
}

publish_log_file(){
    local source_log_file_path="$1"
    local log_name="$2"
    local published_log_file_path=""

    if [[ -z "${source_log_file_path}" ]]; then
        echo "missing source_log_file_path" >&2
        return 1
    fi

    if [[ -z "${log_name}" ]]; then
        echo "missing log_name" >&2
        return 1
    fi

    if [[ ! -f "${source_log_file_path}" ]]; then
        echo "source log file does not exist: ${source_log_file_path}" >&2
        return 1
    fi

    published_log_file_path="$(create_log_file "${log_name}")"
    cat "${source_log_file_path}" > "${published_log_file_path}"

    printf '%s\n' "${published_log_file_path}"
}
