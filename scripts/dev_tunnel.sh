#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"
lib_dir="${script_dir}/lib"

# shellcheck disable=SC1091
source "${lib_dir}/env.sh"
# shellcheck disable=SC1091
source "${lib_dir}/judge_runtime.sh"
# shellcheck disable=SC1091
source "${lib_dir}/frontend_runtime.sh"
# shellcheck disable=SC1091
source "${lib_dir}/process_supervisor.sh"

backend_dir="${project_root}/backend"
frontend_dir="${project_root}/frontend"
backend_env_file="${backend_dir}/.env"
backend_env_example_file="${backend_dir}/.env.example"
frontend_env_local="${frontend_dir}/.env.local"
backend_build_script="${backend_dir}/scripts/build_backend.sh"
backend_http_server_bin="${backend_dir}/http_server"
backend_judge_server_bin="${backend_dir}/judge_server"

frontend_port="${FRONTEND_PORT:-5173}"
frontend_url="http://127.0.0.1:${frontend_port}"
frontend_bin_dir="${FRONTEND_BIN_DIR:-}"
frontend_node_bin="${FRONTEND_NODE_BIN:-node}"
frontend_npm_bin="${FRONTEND_NPM_BIN:-npm}"
cloudflared_bin="${CLOUDFLARED_BIN:-cloudflared}"
cloudflared_timeout="${CLOUDFLARED_TIMEOUT:-45}"
backend_ready_timeout="${BACKEND_READY_TIMEOUT:-30}"
frontend_ready_timeout="${FRONTEND_READY_TIMEOUT:-30}"
backend_build_parallel="${BACKEND_BUILD_PARALLEL:-2}"
dev_tunnel_build_backend="${DEV_TUNNEL_BUILD_BACKEND:-0}"

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

cleanup(){
    cleanup_processes \
        "${run_dir}" \
        "${http_log}" \
        "${judge_log}" \
        "${frontend_log}" \
        "${tunnel_log}" \
        -- \
        "${cloudflared_pid:-}" \
        "${frontend_pid:-}" \
        "${judge_server_pid:-}" \
        "${http_server_pid:-}"
}

ensure_backend_binaries(){
    local should_build=0

    if [[ "${dev_tunnel_build_backend}" == "1" ]]; then
        should_build=1
    fi

    if (( should_build == 0 )) && [[ ! -x "${backend_http_server_bin}" || ! -x "${backend_judge_server_bin}" ]]; then
        should_build=1
    fi

    if (( should_build == 0 )); then
        return 0
    fi

    require_file "${backend_build_script}"

    echo "building backend binaries (parallel=${backend_build_parallel})"
    "${backend_build_script}" http_server judge_server
}

trap handle_signal INT TERM
trap cleanup EXIT

prepend_frontend_bin_dir "${frontend_bin_dir}"
check_frontend_runtime "${frontend_node_bin}" "${frontend_npm_bin}"
load_env_file "${backend_env_file}" "${backend_env_example_file}"

export HTTP_PORT="${HTTP_PORT:-8080}"

require_command curl
require_command "${cloudflared_bin}"
ensure_backend_binaries
require_file "${backend_http_server_bin}"
require_file "${backend_judge_server_bin}"
ensure_judge_runtime_env

if [[ ! -d "${frontend_dir}/node_modules" ]]; then
    echo "error: frontend dependencies are missing: ${frontend_dir}/node_modules" >&2
    echo "hint: run 'cd frontend && npm install'" >&2
    exit 1
fi

echo "starting http_server"
start_service http_server_pid "http_server" "${http_log}" -- \
    /bin/bash -lc 'cd "$1" && exec ./http_server' _ "${backend_dir}"

wait_for_http \
    "http_server" \
    "http://127.0.0.1:${HTTP_PORT}/api/system/health" \
    "${http_server_pid}" \
    "${http_log}" \
    "${backend_ready_timeout}"

echo "starting judge_server"
start_service judge_server_pid "judge_server" "${judge_log}" -- \
    /bin/bash -lc 'cd "$1" && exec ./judge_server' _ "${backend_dir}"

wait_for_process_stable "judge_server" "${judge_server_pid}" "${judge_log}" 3

echo "starting cloudflared quick tunnel"
start_service cloudflared_pid "cloudflared" "${tunnel_log}" -- \
    "${cloudflared_bin}" tunnel --url "${frontend_url}" --no-autoupdate

tunnel_url="$(wait_for_tunnel_url "${cloudflared_pid}" "${tunnel_log}" "${cloudflared_timeout}")"
tunnel_host="${tunnel_url#https://}"
vite_allowed_hosts="$(build_allowed_hosts_csv "${frontend_env_local}" "${tunnel_host}")"

echo "starting frontend dev server"
start_service frontend_pid "frontend dev server" "${frontend_log}" -- \
    env "VITE_ALLOWED_HOSTS=${vite_allowed_hosts}" \
    /bin/bash -lc 'cd "$1" && exec "$2" run dev' _ "${frontend_dir}" "${frontend_npm_bin}"

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

monitor_processes \
    "${http_server_pid}:http_server:${http_log}" \
    "${judge_server_pid}:judge_server:${judge_log}" \
    "${frontend_pid}:frontend dev server:${frontend_log}" \
    "${cloudflared_pid}:cloudflared:${tunnel_log}"
