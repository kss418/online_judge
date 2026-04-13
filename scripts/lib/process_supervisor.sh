#!/usr/bin/env bash

print_error_with_log(){
    local message="$1"
    local log_path="$2"

    echo "error: ${message}" >&2
    if [[ -f "${log_path}" ]]; then
        echo "log: ${log_path}" >&2
        tail -n 40 "${log_path}" >&2 || true
    fi
}

start_service(){
    local pid_var_name="$1"
    local service_name="$2"
    local log_path="$3"

    shift 3

    if [[ "${1:-}" != "--" ]]; then
        echo "error: start_service requires -- before the command for ${service_name}" >&2
        return 1
    fi
    shift

    if (( $# == 0 )); then
        echo "error: missing command for ${service_name}" >&2
        return 1
    fi

    "$@" >"${log_path}" 2>&1 &
    printf -v "${pid_var_name}" '%s' "$!"
}

wait_for_http(){
    local service_name="$1"
    local url="$2"
    local pid="$3"
    local log_path="$4"
    local timeout_seconds="$5"
    local deadline=$((SECONDS + timeout_seconds))

    while (( SECONDS < deadline )); do
        if ! kill -0 "${pid}" >/dev/null 2>&1; then
            print_error_with_log "${service_name} stopped before it became ready" "${log_path}"
            return 1
        fi

        if curl -fsS "${url}" >/dev/null 2>&1; then
            return 0
        fi

        sleep 1
    done

    print_error_with_log "${service_name} did not become ready within ${timeout_seconds}s" "${log_path}"
    return 1
}

wait_for_process_stable(){
    local service_name="$1"
    local pid="$2"
    local log_path="$3"
    local stable_seconds="$4"
    local waited=0

    while (( waited < stable_seconds )); do
        if ! kill -0 "${pid}" >/dev/null 2>&1; then
            print_error_with_log "${service_name} stopped during startup" "${log_path}"
            return 1
        fi

        sleep 1
        waited=$((waited + 1))
    done
}

wait_for_tunnel_url(){
    local pid="$1"
    local log_path="$2"
    local timeout_seconds="$3"
    local deadline=$((SECONDS + timeout_seconds))
    local tunnel_url=""

    while (( SECONDS < deadline )); do
        if ! kill -0 "${pid}" >/dev/null 2>&1; then
            print_error_with_log "cloudflared stopped before publishing a tunnel URL" "${log_path}"
            return 1
        fi

        tunnel_url="$(
            grep -Eo 'https://[[:alnum:].-]+\.trycloudflare\.com' "${log_path}" \
                | tail -n 1 \
                || true
        )"

        if [[ -n "${tunnel_url}" ]]; then
            printf '%s\n' "${tunnel_url}"
            return 0
        fi

        sleep 1
    done

    print_error_with_log "cloudflared did not publish a tunnel URL within ${timeout_seconds}s" "${log_path}"
    return 1
}

monitor_processes(){
    local spec=""
    local pid=""
    local service_name=""
    local log_path=""
    local rest=""

    while true; do
        for spec in "$@"; do
            pid="${spec%%:*}"
            rest="${spec#*:}"
            service_name="${rest%%:*}"
            log_path="${rest#*:}"

            if ! kill -0 "${pid}" >/dev/null 2>&1; then
                print_error_with_log "${service_name} stopped unexpectedly" "${log_path}"
                return 1
            fi
        done

        sleep 2
    done
}

cleanup_processes(){
    local exit_code=$?
    local run_dir="$1"
    local keep_logs="${KEEP_DEV_TUNNEL_LOGS:-0}"
    local has_log_output=0
    local -a log_paths=()
    local -a pids=()
    local log_path=""
    local pid=""

    shift
    trap - EXIT INT TERM

    while (( $# > 0 )); do
        if [[ "$1" == "--" ]]; then
            shift
            break
        fi

        log_paths+=("$1")
        shift
    done

    pids=("$@")

    for pid in "${pids[@]}"; do
        if [[ -n "${pid}" ]] && kill -0 "${pid}" >/dev/null 2>&1; then
            kill "${pid}" >/dev/null 2>&1 || true
        fi
    done

    for pid in "${pids[@]}"; do
        if [[ -n "${pid}" ]]; then
            wait "${pid}" >/dev/null 2>&1 || true
        fi
    done

    for log_path in "${log_paths[@]}"; do
        if [[ -s "${log_path}" ]]; then
            has_log_output=1
            break
        fi
    done

    if [[ "${keep_logs}" != "1" ]] && { (( exit_code == 0 )) || (( has_log_output == 0 )); }; then
        rm -rf "${run_dir}"
        return 0
    fi

    echo "logs kept in ${run_dir}" >&2
}
