#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"
backend_dir="${project_root}/backend"
frontend_dir="${project_root}/frontend"
backend_env_file="${backend_dir}/.env"
frontend_env_local="${frontend_dir}/.env.local"

frontend_port="${FRONTEND_PORT:-5173}"
frontend_url="http://127.0.0.1:${frontend_port}"
frontend_bin_dir="${FRONTEND_BIN_DIR:-}"
frontend_node_bin="${FRONTEND_NODE_BIN:-node}"
frontend_npm_bin="${FRONTEND_NPM_BIN:-npm}"
cloudflared_bin="${CLOUDFLARED_BIN:-cloudflared}"
cloudflared_timeout="${CLOUDFLARED_TIMEOUT:-45}"
backend_ready_timeout="${BACKEND_READY_TIMEOUT:-30}"
frontend_ready_timeout="${FRONTEND_READY_TIMEOUT:-30}"

run_dir="$(mktemp -d /tmp/online_judge_dev_tunnel.XXXXXX)"
http_log="${run_dir}/http_server.log"
judge_log="${run_dir}/judge_server.log"
frontend_log="${run_dir}/frontend.log"
tunnel_log="${run_dir}/cloudflared.log"

http_server_pid=""
judge_server_pid=""
frontend_pid=""
cloudflared_pid=""

handle_signal(){
    exit 0
}

print_error_with_log(){
    local message="$1"
    local log_path="$2"

    echo "error: ${message}" >&2
    if [[ -f "${log_path}" ]]; then
        echo "log: ${log_path}" >&2
        tail -n 40 "${log_path}" >&2 || true
    fi
}

cleanup(){
    local exit_code=$?
    local keep_logs="${KEEP_DEV_TUNNEL_LOGS:-0}"
    local has_log_output=0
    trap - EXIT INT TERM

    for pid in \
        "${cloudflared_pid:-}" \
        "${frontend_pid:-}" \
        "${judge_server_pid:-}" \
        "${http_server_pid:-}"
    do
        if [[ -n "${pid}" ]] && kill -0 "${pid}" >/dev/null 2>&1; then
            kill "${pid}" >/dev/null 2>&1 || true
        fi
    done

    for pid in \
        "${cloudflared_pid:-}" \
        "${frontend_pid:-}" \
        "${judge_server_pid:-}" \
        "${http_server_pid:-}"
    do
        if [[ -n "${pid}" ]]; then
            wait "${pid}" >/dev/null 2>&1 || true
        fi
    done

    for log_path in \
        "${http_log}" \
        "${judge_log}" \
        "${frontend_log}" \
        "${tunnel_log}"
    do
        if [[ -s "${log_path}" ]]; then
            has_log_output=1
            break
        fi
    done

    if [[ "${keep_logs}" != "1" ]] && { (( exit_code == 0 )) || (( has_log_output == 0 )); }; then
        rm -rf "${run_dir}"
        return
    fi

    echo "logs kept in ${run_dir}" >&2
}

trap handle_signal INT TERM
trap cleanup EXIT

require_command(){
    local command_name="$1"

    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "error: required command not found: ${command_name}" >&2
        exit 1
    fi
}

require_file(){
    local file_path="$1"

    if [[ ! -x "${file_path}" ]]; then
        echo "error: required executable not found: ${file_path}" >&2
        exit 1
    fi
}

resolve_command_path(){
    local command_name="$1"
    command -v "${command_name}" 2>/dev/null || true
}

resolve_cpp_compiler_path(){
    local compiler_path=""

    compiler_path="$(resolve_command_path c++)"
    if [[ -n "${compiler_path}" ]]; then
        printf '%s\n' "${compiler_path}"
        return
    fi

    resolve_command_path g++
}

ensure_judge_runtime_env(){
    local missing_requirements=()

    export JUDGE_SOURCE_ROOT="${JUDGE_SOURCE_ROOT:-/tmp/online_judge/submissions}"
    export TESTCASE_PATH="${TESTCASE_PATH:-/tmp/online_judge/testcases}"
    export JUDGE_NSJAIL_PATH="${JUDGE_NSJAIL_PATH:-$(resolve_command_path nsjail)}"
    export JUDGE_CPP_COMPILER_PATH="${JUDGE_CPP_COMPILER_PATH:-$(resolve_cpp_compiler_path)}"
    export JUDGE_PYTHON_PATH="${JUDGE_PYTHON_PATH:-$(resolve_command_path python3)}"
    export JUDGE_JAVA_COMPILER_PATH="${JUDGE_JAVA_COMPILER_PATH:-$(resolve_command_path javac)}"
    export JUDGE_JAVA_RUNTIME_PATH="${JUDGE_JAVA_RUNTIME_PATH:-$(resolve_command_path java)}"

    [[ -n "${JUDGE_NSJAIL_PATH}" && -x "${JUDGE_NSJAIL_PATH}" ]] || missing_requirements+=("nsjail or JUDGE_NSJAIL_PATH")
    [[ -n "${JUDGE_CPP_COMPILER_PATH}" && -x "${JUDGE_CPP_COMPILER_PATH}" ]] || missing_requirements+=("C++ compiler (c++/g++) or JUDGE_CPP_COMPILER_PATH")
    [[ -n "${JUDGE_PYTHON_PATH}" && -x "${JUDGE_PYTHON_PATH}" ]] || missing_requirements+=("Python 3 or JUDGE_PYTHON_PATH")
    [[ -n "${JUDGE_JAVA_COMPILER_PATH}" && -x "${JUDGE_JAVA_COMPILER_PATH}" ]] || missing_requirements+=("Java compiler (javac) or JUDGE_JAVA_COMPILER_PATH")
    [[ -n "${JUDGE_JAVA_RUNTIME_PATH}" && -x "${JUDGE_JAVA_RUNTIME_PATH}" ]] || missing_requirements+=("Java runtime (java) or JUDGE_JAVA_RUNTIME_PATH")

    if (( ${#missing_requirements[@]} > 0 )); then
        echo "error: judge runtime prerequisites are missing" >&2
        printf '  - %s\n' "${missing_requirements[@]}" >&2
        echo "hint: install the missing tools or set the corresponding JUDGE_* paths in backend/.env" >&2
        exit 1
    fi

    mkdir -p "${JUDGE_SOURCE_ROOT}" "${TESTCASE_PATH}"
}

prepend_frontend_bin_dir(){
    if [[ -z "${frontend_bin_dir}" ]]; then
        return
    fi

    if [[ ! -d "${frontend_bin_dir}" ]]; then
        echo "error: FRONTEND_BIN_DIR does not exist: ${frontend_bin_dir}" >&2
        exit 1
    fi

    export PATH="${frontend_bin_dir}:${PATH}"
}

parse_semver(){
    local version_text="${1#v}"

    if [[ ! "${version_text}" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
        return 1
    fi

    printf '%s %s %s\n' \
        "${BASH_REMATCH[1]}" \
        "${BASH_REMATCH[2]}" \
        "${BASH_REMATCH[3]}"
}

node_version_satisfies_frontend(){
    local version_text="$1"
    local major=0
    local minor=0
    local patch=0

    read -r major minor patch < <(parse_semver "${version_text}") || return 1

    if (( major == 20 )); then
        (( minor > 19 )) && return 0
        (( minor == 19 && patch >= 0 )) && return 0
        return 1
    fi

    if (( major == 21 )); then
        return 1
    fi

    if (( major == 22 )); then
        (( minor > 12 )) && return 0
        (( minor == 12 && patch >= 0 )) && return 0
        return 1
    fi

    (( major > 22 ))
}

check_frontend_runtime(){
    require_command "${frontend_node_bin}"
    require_command "${frontend_npm_bin}"

    local node_version
    node_version="$("${frontend_node_bin}" -v)"

    if ! node_version_satisfies_frontend "${node_version}"; then
        echo "error: frontend requires Node.js ^20.19.0 or >=22.12.0, but found ${node_version}" >&2
        echo "node path: $(command -v "${frontend_node_bin}")" >&2
        echo "hint: install Node.js 22.12.0+ or 20.19.0+ before running this script" >&2
        echo "hint: you can also run with FRONTEND_BIN_DIR=/path/to/node/bin ./scripts/dev_tunnel.sh" >&2
        echo "hint: or set FRONTEND_NODE_BIN and FRONTEND_NPM_BIN explicitly" >&2
        exit 1
    fi
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
            exit 1
        fi

        if curl -fsS "${url}" >/dev/null 2>&1; then
            return
        fi

        sleep 1
    done

    print_error_with_log "${service_name} did not become ready within ${timeout_seconds}s" "${log_path}"
    exit 1
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
            exit 1
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

    while (( SECONDS < deadline )); do
        if ! kill -0 "${pid}" >/dev/null 2>&1; then
            print_error_with_log "cloudflared stopped before publishing a tunnel URL" "${log_path}"
            exit 1
        fi

        local tunnel_url
        tunnel_url="$(
            grep -Eo 'https://[[:alnum:].-]+\.trycloudflare\.com' "${log_path}" \
                | tail -n 1 \
                || true
        )"

        if [[ -n "${tunnel_url}" ]]; then
            printf '%s\n' "${tunnel_url}"
            return
        fi

        sleep 1
    done

    print_error_with_log "cloudflared did not publish a tunnel URL within ${timeout_seconds}s" "${log_path}"
    exit 1
}

build_allowed_hosts_csv(){
    local tunnel_host="$1"

    {
        if [[ -n "${VITE_ALLOWED_HOSTS:-}" ]]; then
            printf '%s\n' "${VITE_ALLOWED_HOSTS}" | tr ',' '\n'
        fi

        if [[ -f "${frontend_env_local}" ]]; then
            sed -n 's/^VITE_ALLOWED_HOSTS=//p' "${frontend_env_local}" | tail -n 1 | tr ',' '\n'
        fi

        if [[ -n "${EXTRA_VITE_ALLOWED_HOSTS:-}" ]]; then
            printf '%s\n' "${EXTRA_VITE_ALLOWED_HOSTS}" | tr ',' '\n'
        fi
    } | awk -v tunnel_host="${tunnel_host}" '
        {
            gsub(/^[[:space:]]+|[[:space:]]+$/, "", $0)
            if($0 == "" || $0 ~ /\.trycloudflare\.com$/){
                next
            }

            if(!seen[$0]++){
                hosts[count++] = $0
            }
        }
        END {
            if(tunnel_host != "" && !seen[tunnel_host]++){
                hosts[count++] = tunnel_host
            }

            for(i = 0; i < count; ++i){
                printf("%s", hosts[i])
                if(i + 1 < count){
                    printf(",")
                }
            }
        }
    '
}

monitor_processes(){
    while true; do
        if ! kill -0 "${http_server_pid}" >/dev/null 2>&1; then
            print_error_with_log "http_server stopped unexpectedly" "${http_log}"
            exit 1
        fi

        if ! kill -0 "${judge_server_pid}" >/dev/null 2>&1; then
            print_error_with_log "judge_server stopped unexpectedly" "${judge_log}"
            exit 1
        fi

        if ! kill -0 "${frontend_pid}" >/dev/null 2>&1; then
            print_error_with_log "frontend dev server stopped unexpectedly" "${frontend_log}"
            exit 1
        fi

        if ! kill -0 "${cloudflared_pid}" >/dev/null 2>&1; then
            print_error_with_log "cloudflared stopped unexpectedly" "${tunnel_log}"
            exit 1
        fi

        sleep 2
    done
}

if [[ ! -f "${backend_env_file}" ]]; then
    echo "error: backend env file not found: ${backend_env_file}" >&2
    echo "hint: copy backend/.env.example to backend/.env and fill the values" >&2
    exit 1
fi

prepend_frontend_bin_dir
check_frontend_runtime

set -a
# shellcheck disable=SC1090
source "${backend_env_file}"
set +a

export HTTP_PORT="${HTTP_PORT:-8080}"

require_command curl
require_command "${cloudflared_bin}"
require_file "${backend_dir}/http_server"
require_file "${backend_dir}/judge_server"
ensure_judge_runtime_env

if [[ ! -d "${frontend_dir}/node_modules" ]]; then
    echo "error: frontend dependencies are missing: ${frontend_dir}/node_modules" >&2
    echo "hint: run 'cd frontend && npm install'" >&2
    exit 1
fi

echo "starting http_server"
(
    cd "${backend_dir}"
    exec ./http_server
) >"${http_log}" 2>&1 &
http_server_pid="$!"

wait_for_http \
    "http_server" \
    "http://127.0.0.1:${HTTP_PORT}/api/system/health" \
    "${http_server_pid}" \
    "${http_log}" \
    "${backend_ready_timeout}"

echo "starting judge_server"
(
    cd "${backend_dir}"
    exec ./judge_server
) >"${judge_log}" 2>&1 &
judge_server_pid="$!"

wait_for_process_stable "judge_server" "${judge_server_pid}" "${judge_log}" 3

echo "starting cloudflared quick tunnel"
"${cloudflared_bin}" tunnel --url "${frontend_url}" --no-autoupdate >"${tunnel_log}" 2>&1 &
cloudflared_pid="$!"

tunnel_url="$(wait_for_tunnel_url "${cloudflared_pid}" "${tunnel_log}" "${cloudflared_timeout}")"
tunnel_host="${tunnel_url#https://}"
vite_allowed_hosts="$(build_allowed_hosts_csv "${tunnel_host}")"

echo "starting frontend dev server"
(
    cd "${frontend_dir}"
    export VITE_ALLOWED_HOSTS="${vite_allowed_hosts}"
    exec "${frontend_npm_bin}" run dev
) >"${frontend_log}" 2>&1 &
frontend_pid="$!"

wait_for_http \
    "frontend dev server" \
    "${frontend_url}" \
    "${frontend_pid}" \
    "${frontend_log}" \
    "${frontend_ready_timeout}"

cat <<EOF
all services are running

frontend(local): ${frontend_url}
frontend(public): ${tunnel_url}
backend health: http://127.0.0.1:${HTTP_PORT}/api/system/health
vite allowed hosts: ${vite_allowed_hosts}
logs: ${run_dir}

press Ctrl+C to stop everything
EOF

monitor_processes
