#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/util.sh"
# shellcheck disable=SC1091
source "${script_dir}/flow_test_util.sh"
# shellcheck disable=SC1091
source "${script_dir}/database_util.sh"
# shellcheck disable=SC1091
source "${script_dir}/http_server_util.sh"
# shellcheck disable=SC1091
source "${script_dir}/fixture_util.sh"

if [[ -f "${project_root}/.env" ]]; then
    set -a
    # shellcheck disable=SC1091
    source "${project_root}/.env"
    set +a
fi

http_port="${HTTP_RUNTIME_LIMITS_FLOW_TEST_HTTP_PORT:-18086}"
base_url="${HTTP_RUNTIME_LIMITS_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${HTTP_RUNTIME_LIMITS_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
test_db_name="http_runtime_limits_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_http_runtime_limits_flow.log"
server_log_name="test_http_runtime_limits_flow_server.log"

init_flow_test
register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file runtime_response_file

trap 'finish_flow_test cleanup_http_server drop_test_database' EXIT

require_command curl
require_command psql
require_command python3

export DB_ADMIN_USER="${DB_ADMIN_USER:-${DB_USER:-postgres}}"
export DB_ADMIN_PASSWORD="${DB_ADMIN_PASSWORD:-${DB_PASSWORD:-postgres}}"
test_database_url="postgresql://${DB_USER}:${DB_PASSWORD}@${DB_HOST}:${DB_PORT}/${test_db_name}"
create_test_database
export DB_NAME="${test_db_name}"

append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"

apply_test_database_migrations

clear_http_runtime_overrides(){
    unset HTTP_WORKER_COUNT
    unset HTTP_HANDLER_WORKER_COUNT
    unset HTTP_DB_POOL_SIZE
    unset HTTP_DB_ACQUIRE_TIMEOUT_MS
    unset HTTP_HANDLER_QUEUE_LIMIT
    unset HTTP_REQUEST_DEADLINE_MS
    unset HTTP_KEEP_ALIVE_IDLE_TIMEOUT_MS
}

start_runtime_limited_http_server(){
    local port="$1"

    http_port="${port}"
    base_url="http://127.0.0.1:${http_port}"
    : > "${server_log_temp_file}"
    ensure_dedicated_http_server
}

run_invalid_startup_case(){
    local env_name="$1"
    local env_value="$2"
    local failure_context="$3"
    local exit_code=0

    clear_http_runtime_overrides
    append_log_line "${test_log_temp_file}" "invalid startup case: ${env_name}=${env_value}"
    : > "${server_log_temp_file}"

    if env \
        "HTTP_PORT=${http_port}" \
        "${env_name}=${env_value}" \
        "${http_server_bin}" >"${server_log_temp_file}" 2>&1; then
        append_log_line "${test_log_temp_file}" "${failure_context} unexpectedly started"
        publish_failure_logs
        echo "${failure_context}: expected startup failure, but server started" >&2
        exit 1
    else
        exit_code=$?
    fi

    if [[ "${exit_code}" == "0" ]]; then
        append_log_line "${test_log_temp_file}" "${failure_context} exit code was zero"
        publish_failure_logs
        echo "${failure_context}: expected non-zero exit code" >&2
        exit 1
    fi

    print_success_log "${failure_context} startup failure validated"
}

assert_queue_full_response(){
    local failure_context="$1"

    if ! python3 - "${base_url}" <<'PY'
import concurrent.futures
import http.client
import json
import sys
import urllib.parse

base_url = sys.argv[1]
parsed = urllib.parse.urlparse(base_url)
host = parsed.hostname or "127.0.0.1"
port = parsed.port or 80
path = "/api/system/health"
expected_message = "handler queue is full, retry later"

def send_once(_):
    conn = http.client.HTTPConnection(host, port, timeout=5)
    try:
        conn.request("GET", path, headers={"Connection": "close"})
        response = conn.getresponse()
        body = response.read().decode("utf-8")
        return response.status, body
    finally:
        conn.close()

queue_full_count = 0
for _ in range(12):
    with concurrent.futures.ThreadPoolExecutor(max_workers=128) as executor:
        for status, body in executor.map(send_once, range(256)):
            if status != 503:
                continue
            try:
                payload = json.loads(body)
            except json.JSONDecodeError:
                continue
            if payload.get("error", {}).get("message") == expected_message:
                queue_full_count += 1
    if queue_full_count > 0:
        print(queue_full_count)
        raise SystemExit(0)

raise SystemExit("queue full response not observed")
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} failed"
        publish_failure_logs
        echo "${failure_context}: expected queue full response" >&2
        exit 1
    fi

    print_success_log "${failure_context} validated"
}

assert_request_deadline_response(){
    local failure_context="$1"

    if ! python3 - "${base_url}" <<'PY'
import concurrent.futures
import http.client
import json
import sys
import urllib.parse

base_url = sys.argv[1]
parsed = urllib.parse.urlparse(base_url)
host = parsed.hostname or "127.0.0.1"
port = parsed.port or 80
path = "/api/problem"
expected_message = "request deadline exceeded, retry later"

def send_once(_):
    conn = http.client.HTTPConnection(host, port, timeout=5)
    try:
        conn.request("GET", path, headers={"Connection": "close"})
        response = conn.getresponse()
        body = response.read().decode("utf-8")
        return response.status, body
    except Exception:
        return None, ""
    finally:
        conn.close()

deadline_count = 0
for _ in range(16):
    with concurrent.futures.ThreadPoolExecutor(max_workers=128) as executor:
        for status, body in executor.map(send_once, range(256)):
            if status != 503:
                continue
            try:
                payload = json.loads(body)
            except json.JSONDecodeError:
                continue
            if payload.get("error", {}).get("message") == expected_message:
                deadline_count += 1

    if deadline_count > 0:
        print(deadline_count)
        raise SystemExit(0)

raise SystemExit("request deadline response not observed")
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} failed"
        publish_failure_logs
        echo "${failure_context}: expected request deadline response" >&2
        exit 1
    fi

    print_success_log "${failure_context} validated"
}

assert_keep_alive_idle_timeout(){
    local failure_context="$1"

    if ! python3 - "${base_url}" <<'PY'
import socket
import sys
import time
import urllib.parse

base_url = sys.argv[1]
parsed = urllib.parse.urlparse(base_url)
host = parsed.hostname or "127.0.0.1"
port = parsed.port or 80

sock = socket.create_connection((host, port), timeout=5)
sock.settimeout(5)
try:
    first_request = (
        "GET /api/system/health HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
    )
    sock.sendall(first_request.encode("utf-8"))

    response = b""
    while b"\r\n\r\n" not in response:
        response += sock.recv(4096)

    header_bytes, _, body_bytes = response.partition(b"\r\n\r\n")
    content_length = 0
    for raw_line in header_bytes.decode("utf-8", errors="replace").split("\r\n"):
        if raw_line.lower().startswith("content-length:"):
            content_length = int(raw_line.split(":", 1)[1].strip())
            break

    while len(body_bytes) < content_length:
        body_bytes += sock.recv(4096)

    time.sleep(0.4)

    second_request = (
        "GET /api/system/health HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        "Connection: close\r\n"
        "\r\n"
    )

    try:
        sock.sendall(second_request.encode("utf-8"))
    except (BrokenPipeError, ConnectionResetError):
        raise SystemExit(0)

    data = sock.recv(4096)
    if data == b"":
        raise SystemExit(0)

    raise SystemExit("connection remained open after keep-alive idle timeout")
finally:
    sock.close()
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} failed"
        publish_failure_logs
        echo "${failure_context}: expected keep-alive idle timeout to close the connection" >&2
        exit 1
    fi

    print_success_log "${failure_context} validated"
}

clear_http_runtime_overrides
run_invalid_startup_case "HTTP_DB_ACQUIRE_TIMEOUT_MS" "abc" "invalid db acquire timeout"
run_invalid_startup_case "HTTP_HANDLER_QUEUE_LIMIT" "-1" "invalid handler queue limit"
run_invalid_startup_case "HTTP_REQUEST_DEADLINE_MS" "not-a-number" "invalid request deadline"
run_invalid_startup_case "HTTP_KEEP_ALIVE_IDLE_TIMEOUT_MS" "bad" "invalid keep-alive idle timeout"

clear_http_runtime_overrides
export HTTP_WORKER_COUNT=8
export HTTP_HANDLER_WORKER_COUNT=1
export HTTP_HANDLER_QUEUE_LIMIT=1
export HTTP_REQUEST_DEADLINE_MS=0
export HTTP_KEEP_ALIVE_IDLE_TIMEOUT_MS=0
start_runtime_limited_http_server 18086
assert_queue_full_response "handler queue full"
cleanup_http_server

clear_http_runtime_overrides
export HTTP_REQUEST_DEADLINE_MS=1
export HTTP_WORKER_COUNT=8
export HTTP_HANDLER_WORKER_COUNT=8
export HTTP_HANDLER_QUEUE_LIMIT=0
export HTTP_DB_POOL_SIZE=1
export HTTP_KEEP_ALIVE_IDLE_TIMEOUT_MS=0
start_runtime_limited_http_server 18088
assert_request_deadline_response "request deadline"
cleanup_http_server

clear_http_runtime_overrides
export HTTP_REQUEST_DEADLINE_MS=0
export HTTP_KEEP_ALIVE_IDLE_TIMEOUT_MS=150
start_runtime_limited_http_server 18090
assert_keep_alive_idle_timeout "keep-alive idle timeout"
cleanup_http_server

clear_http_runtime_overrides
append_log_line "${test_log_temp_file}" "http runtime limits flow test passed"
print_success_log "http runtime limits flow test passed"
