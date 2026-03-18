#!/usr/bin/env bash

require_http_server_test_env(){
    local missing_vars=()

    [[ -n "${base_url:-}" ]] || missing_vars+=("base_url")
    [[ -n "${http_server_bin:-}" ]] || missing_vars+=("http_server_bin")
    [[ -n "${http_port:-}" ]] || missing_vars+=("http_port")
    [[ -n "${project_root:-}" ]] || missing_vars+=("project_root")
    [[ -n "${test_log_temp_file:-}" ]] || missing_vars+=("test_log_temp_file")
    [[ -n "${server_log_temp_file:-}" ]] || missing_vars+=("server_log_temp_file")
    [[ -n "${test_log_name:-}" ]] || missing_vars+=("test_log_name")
    [[ -n "${server_log_name:-}" ]] || missing_vars+=("server_log_name")

    if (( ${#missing_vars[@]} > 0 )); then
        printf 'missing required http server test vars: %s\n' "${missing_vars[*]}" >&2
        return 1
    fi
}

cleanup_http_server(){
    if [[ -n "${server_pid:-}" ]]; then
        kill "${server_pid}" >/dev/null 2>&1 || true
        wait "${server_pid}" >/dev/null 2>&1 || true
    fi
}

health_check(){
    curl --silent --show-error --fail "${base_url}/api/system/health" >/dev/null 2>&1
}

wait_for_health(){
    local attempt=0

    while (( attempt < 50 )); do
        if health_check; then
            return 0
        fi

        sleep 0.2
        attempt=$((attempt + 1))
    done

    return 1
}

publish_failure_logs(){
    require_http_server_test_env

    if [[ -z "${test_log_path:-}" ]]; then
        test_log_path="$(publish_log_file "${test_log_temp_file}" "${test_log_name}")"
        print_log_file_created "${test_log_path}"
    fi

    if [[ -z "${server_log_path:-}" ]] && [[ -n "${server_pid:-}" || -s "${server_log_temp_file}" ]]; then
        server_log_path="$(publish_log_file "${server_log_temp_file}" "${server_log_name}")"
        print_log_file_created "${server_log_path}"
    fi
}

ensure_http_server(){
    require_http_server_test_env

    if ! health_check; then
        if [[ ! -x "${http_server_bin}" ]]; then
            echo "http_server binary not found or not executable: ${http_server_bin}" >&2
            append_log_line "${test_log_temp_file}" "http_server binary not found: ${http_server_bin}"
            publish_failure_logs
            echo "hint: run 'cmake --build ${project_root}/build'" >&2
            return 1
        fi

        append_log_line "${test_log_temp_file}" "starting local http_server"
        HTTP_PORT="${http_port}" "${http_server_bin}" >"${server_log_temp_file}" 2>&1 &
        server_pid="$!"

        if ! wait_for_health; then
            echo "failed to start http_server" >&2
            append_log_line "${test_log_temp_file}" "failed to start local http_server"
            publish_failure_logs
            echo "server log:" >&2
            cat "${server_log_temp_file}" >&2
            return 1
        fi
    else
        append_log_line "${test_log_temp_file}" "reusing existing http_server"
    fi
}
