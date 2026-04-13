#!/usr/bin/env bash

require_command(){
    local command_name="$1"

    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "error: required command not found: ${command_name}" >&2
        return 1
    fi
}

require_file(){
    local file_path="$1"

    if [[ ! -x "${file_path}" ]]; then
        echo "error: required executable not found: ${file_path}" >&2
        return 1
    fi
}

resolve_command_path(){
    local command_name="$1"

    command -v "${command_name}" 2>/dev/null || true
}

load_env_file(){
    local env_file_path="$1"
    local example_hint_path="$2"
    local display_env_file_path="${env_file_path}"
    local display_example_hint_path="${example_hint_path}"
    local restore_allexport=0

    if [[ "${display_env_file_path}" == "${PWD}/"* ]]; then
        display_env_file_path="${display_env_file_path#${PWD}/}"
    fi

    if [[ "${display_example_hint_path}" == "${PWD}/"* ]]; then
        display_example_hint_path="${display_example_hint_path#${PWD}/}"
    fi

    if [[ ! -f "${env_file_path}" ]]; then
        echo "error: backend env file not found: ${env_file_path}" >&2
        if [[ -n "${example_hint_path}" ]]; then
            echo "hint: copy ${display_example_hint_path} to ${display_env_file_path} and fill the values" >&2
        fi
        return 1
    fi

    if [[ "${-}" == *a* ]]; then
        restore_allexport=1
    else
        set -a
    fi

    # shellcheck disable=SC1090
    source "${env_file_path}"

    if (( restore_allexport == 0 )); then
        set +a
    fi
}
