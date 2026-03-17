#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/test_util.sh"

if [[ -f "${project_root}/.env" ]]; then
    set -a
    # shellcheck disable=SC1091
    source "${project_root}/.env"
    set +a
fi

http_port="${HTTP_PORT:-18080}"
base_url="${AUTH_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${AUTH_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${AUTH_FLOW_TEST_LOGIN_ID:-auth_flow_test_$(date +%s)_$$}"
raw_password="${AUTH_FLOW_TEST_PASSWORD:-password123}"
test_log_path=""
server_log_path=""
server_pid=""
test_log_temp_file="$(mktemp)"
server_log_temp_file="$(mktemp)"
sign_up_response_file="$(mktemp)"
login_response_file="$(mktemp)"
renew_response_file="$(mktemp)"
logout_response_file="$(mktemp)"
second_logout_response_file="$(mktemp)"

cleanup(){
    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${sign_up_response_file}" \
        "${login_response_file}" \
        "${renew_response_file}" \
        "${logout_response_file}" \
        "${second_logout_response_file}"

    if [[ -n "${server_pid}" ]]; then
        kill "${server_pid}" >/dev/null 2>&1 || true
        wait "${server_pid}" >/dev/null 2>&1 || true
    fi
}

require_command(){
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "missing command: $1" >&2
        exit 1
    fi
}

health_check(){
    curl --silent --show-error --fail "${base_url}/api/health" >/dev/null 2>&1
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
    if [[ -z "${test_log_path}" ]]; then
        test_log_path="$(publish_log_file "${test_log_temp_file}" "test_auth_flow.log")"
        print_log_file_created "${test_log_path}"
    fi

    if [[ -z "${server_log_path}" ]] && [[ -n "${server_pid}" || -s "${server_log_temp_file}" ]]; then
        server_log_path="$(publish_log_file "${server_log_temp_file}" "test_auth_flow_server.log")"
        print_log_file_created "${server_log_path}"
    fi
}

print_success_log(){
    local log_message="$1"

    if [[ -z "${log_message}" ]]; then
        echo "missing log_message" >&2
        return 1
    fi

    printf '%s\n' "${log_message}"
    append_log_line "${test_log_temp_file}" "${log_message}"
}

trap cleanup EXIT

require_command curl
require_command python3

append_log_line "${test_log_temp_file}" "base_url=${base_url}"
append_log_line "${test_log_temp_file}" "user_login_id=${user_login_id}"

if ! health_check; then
    if [[ ! -x "${http_server_bin}" ]]; then
        echo "http_server binary not found or not executable: ${http_server_bin}" >&2
        append_log_line "${test_log_temp_file}" "http_server binary not found: ${http_server_bin}"
        publish_failure_logs
        echo "hint: run 'cmake --build ${project_root}/build'" >&2
        exit 1
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
        exit 1
    fi
else
    append_log_line "${test_log_temp_file}" "reusing existing http_server"
fi

request_body="$(
    python3 - "${user_login_id}" "${raw_password}" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "user_login_id": sys.argv[1],
            "raw_password": sys.argv[2],
        }
    )
)
PY
)"

sign_up_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${sign_up_response_file}" \
        --write-out "%{http_code}" \
        -H "Content-Type: application/json" \
        -d "${request_body}" \
        "${base_url}/api/sign-up"
)"

if [[ "${sign_up_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "sign-up failed: status=${sign_up_status_code}"
    publish_failure_logs
    echo "sign-up test failed: expected status 201, got ${sign_up_status_code}" >&2
    echo "response body:" >&2
    cat "${sign_up_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "sign-up passed: status=${sign_up_status_code}"
print_success_log "sign-up success"

login_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${login_response_file}" \
        --write-out "%{http_code}" \
        -H "Content-Type: application/json" \
        -d "${request_body}" \
        "${base_url}/api/login"
)"

if [[ "${login_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "login failed: status=${login_status_code}"
    publish_failure_logs
    echo "login test failed: expected status 200, got ${login_status_code}" >&2
    echo "response body:" >&2
    cat "${login_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "login passed: status=${login_status_code}"
print_success_log "login success"

if ! python3 - "${sign_up_response_file}" "${login_response_file}" "${user_login_id}" <<'PY'
import json
import sys

sign_up_response_file_path = sys.argv[1]
login_response_file_path = sys.argv[2]
expected_login_id = sys.argv[3]

with open(sign_up_response_file_path, encoding="utf-8") as response_file:
    sign_up_response = json.load(response_file)

with open(login_response_file_path, encoding="utf-8") as response_file:
    login_response = json.load(response_file)

sign_up_user_id = sign_up_response.get("user_id")
sign_up_is_admin = sign_up_response.get("is_admin")
sign_up_token = sign_up_response.get("token")

login_user_id = login_response.get("user_id")
login_is_admin = login_response.get("is_admin")
login_token = login_response.get("token")

if not isinstance(sign_up_user_id, int) or sign_up_user_id <= 0:
    raise SystemExit("invalid user_id in sign-up response")

if sign_up_is_admin is not False:
    raise SystemExit("expected is_admin to be false for new user")

if not isinstance(sign_up_token, str) or not sign_up_token:
    raise SystemExit("missing token in sign-up response")

if login_user_id != sign_up_user_id:
    raise SystemExit("login response user_id does not match sign-up response")

if login_is_admin is not False:
    raise SystemExit("expected is_admin to stay false after login")

if not isinstance(login_token, str) or not login_token:
    raise SystemExit("missing token in login response")

if login_token == sign_up_token:
    raise SystemExit("expected login to issue a new token")
PY
then
    append_log_line "${test_log_temp_file}" "response validation failed"
    publish_failure_logs
    exit 1
fi

append_log_line "${test_log_temp_file}" "auth flow test passed"

login_user_id="$(
    python3 - "${login_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    login_response = json.load(response_file)

user_id = login_response.get("user_id")
if not isinstance(user_id, int) or user_id <= 0:
    raise SystemExit("invalid user_id in login response")

print(user_id)
PY
)"

login_token="$(
    python3 - "${login_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    login_response = json.load(response_file)

login_token = login_response.get("token")
if not isinstance(login_token, str) or not login_token:
    raise SystemExit("missing token in login response")

print(login_token)
PY
)"

renew_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${renew_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${login_token}" \
        "${base_url}/api/token/renew"
)"

if [[ "${renew_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "token renew failed: status=${renew_status_code}"
    publish_failure_logs
    echo "token renew test failed: expected status 200, got ${renew_status_code}" >&2
    echo "response body:" >&2
    cat "${renew_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "token renew passed: status=${renew_status_code}"
print_success_log "token renew success"

logout_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${logout_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${login_token}" \
        "${base_url}/api/logout"
)"

if [[ "${logout_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "logout failed: status=${logout_status_code}"
    publish_failure_logs
    echo "logout test failed: expected status 200, got ${logout_status_code}" >&2
    echo "response body:" >&2
    cat "${logout_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "logout passed: status=${logout_status_code}"
print_success_log "logout success"

second_logout_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${second_logout_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${login_token}" \
        "${base_url}/api/logout"
)"

if [[ "${second_logout_status_code}" != "401" ]]; then
    append_log_line "${test_log_temp_file}" "second logout failed: status=${second_logout_status_code}"
    publish_failure_logs
    echo "second logout test failed: expected status 401, got ${second_logout_status_code}" >&2
    echo "response body:" >&2
    cat "${second_logout_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "second logout passed: status=${second_logout_status_code}"
print_success_log "token reuse failure success"
print_success_log "auth flow test passed: login_id=${user_login_id}, user_id=${login_user_id}"
