#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/util.sh"
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

http_port="${AUTH_FLOW_TEST_HTTP_PORT:-18080}"
base_url="${AUTH_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${AUTH_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${AUTH_FLOW_TEST_LOGIN_ID:-auth_flow_test_$(date +%s)_$$}"
second_user_login_id="${AUTH_FLOW_TEST_SECOND_LOGIN_ID:-${user_login_id}_second}"
raw_password="${AUTH_FLOW_TEST_PASSWORD:-password123}"
test_db_name="auth_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_auth_flow.log"
server_log_name="test_auth_flow_server.log"
test_log_temp_file="$(mktemp)"
server_log_temp_file="$(mktemp)"
sign_up_response_file="$(mktemp)"
login_response_file="$(mktemp)"
renew_response_file="$(mktemp)"
logout_response_file="$(mktemp)"
second_logout_response_file="$(mktemp)"
second_sign_up_response_file="$(mktemp)"
second_login_response_file="$(mktemp)"
promote_admin_response_file="$(mktemp)"
unauthorized_promote_response_file="$(mktemp)"

cleanup(){
    cleanup_http_server
    drop_test_database

    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${sign_up_response_file}" \
        "${login_response_file}" \
        "${renew_response_file}" \
        "${logout_response_file}" \
        "${second_logout_response_file}" \
        "${second_sign_up_response_file}" \
        "${second_login_response_file}" \
        "${promote_admin_response_file}" \
        "${unauthorized_promote_response_file}"

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
require_command psql
require_command python3

export DB_ADMIN_USER="${DB_ADMIN_USER:-${DB_USER:-postgres}}"
export DB_ADMIN_PASSWORD="${DB_ADMIN_PASSWORD:-${DB_PASSWORD:-postgres}}"
test_database_url="postgresql://${DB_USER}:${DB_PASSWORD}@${DB_HOST}:${DB_PORT}/${test_db_name}"
create_test_database
export DB_NAME="${test_db_name}"

append_log_line "${test_log_temp_file}" "base_url=${base_url}"
append_log_line "${test_log_temp_file}" "user_login_id=${user_login_id}"
append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"

apply_test_database_migrations
ensure_dedicated_http_server

sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "auth flow" >/dev/null
print_success_log "sign-up success"

request_body="$(make_sign_up_request_body "${user_login_id}" "${raw_password}")"

login_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${login_response_file}" \
        --write-out "%{http_code}" \
        -H "Content-Type: application/json" \
        -d "${request_body}" \
        "${base_url}/api/auth/login"
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

read -r second_user_id second_user_token < <(
    sign_up_user "${second_user_login_id}" "${raw_password}" "${second_sign_up_response_file}" "auth flow"
)
print_success_log "second sign-up success"

unauthorized_promote_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${unauthorized_promote_response_file}" \
        --write-out "%{http_code}" \
        --request PUT \
        -H "Authorization: Bearer ${second_user_token}" \
        "${base_url}/api/user/${login_user_id}/admin"
)"

if [[ "${unauthorized_promote_status_code}" != "401" ]]; then
    append_log_line "${test_log_temp_file}" "unauthorized promote failed: status=${unauthorized_promote_status_code}"
    publish_failure_logs
    echo "unauthorized promote test failed: expected status 401, got ${unauthorized_promote_status_code}" >&2
    echo "response body:" >&2
    cat "${unauthorized_promote_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "unauthorized promote passed: status=${unauthorized_promote_status_code}"
print_success_log "unauthorized promote success"

promote_admin_user "${login_user_id}" "auth flow" >/dev/null
print_success_log "bootstrap admin promote success"

promote_admin_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${promote_admin_response_file}" \
        --write-out "%{http_code}" \
        --request PUT \
        -H "Authorization: Bearer ${login_token}" \
        "${base_url}/api/user/${second_user_id}/admin"
)"

if [[ "${promote_admin_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "promote admin failed: status=${promote_admin_status_code}"
    publish_failure_logs
    echo "promote admin test failed: expected status 200, got ${promote_admin_status_code}" >&2
    echo "response body:" >&2
    cat "${promote_admin_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "promote admin passed: status=${promote_admin_status_code}"
print_success_log "promote admin success"

second_login_request_body="$(make_sign_up_request_body "${second_user_login_id}" "${raw_password}")"
second_login_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${second_login_response_file}" \
        --write-out "%{http_code}" \
        -H "Content-Type: application/json" \
        -d "${second_login_request_body}" \
        "${base_url}/api/auth/login"
)"

if [[ "${second_login_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "second login failed: status=${second_login_status_code}"
    publish_failure_logs
    echo "second login test failed: expected status 200, got ${second_login_status_code}" >&2
    echo "response body:" >&2
    cat "${second_login_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "second login passed: status=${second_login_status_code}"
print_success_log "second login success"

if ! python3 \
    - "${unauthorized_promote_response_file}" \
    "${promote_admin_response_file}" \
    "${second_login_response_file}" \
    "${second_user_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    unauthorized_promote_response = json.load(response_file)

with open(sys.argv[2], encoding="utf-8") as response_file:
    promote_admin_response = json.load(response_file)

with open(sys.argv[3], encoding="utf-8") as response_file:
    second_login_response = json.load(response_file)

expected_second_user_id = int(sys.argv[4])

if unauthorized_promote_response.get("error", {}).get("code") != "admin_bearer_token_required":
    raise SystemExit("unexpected error code for unauthorized promote response")

if promote_admin_response.get("user_id") != expected_second_user_id:
    raise SystemExit("promote admin response user_id mismatch")

if promote_admin_response.get("is_admin") is not True:
    raise SystemExit("expected promoted user to be admin in promote admin response")

if second_login_response.get("user_id") != expected_second_user_id:
    raise SystemExit("second login response user_id mismatch")

if second_login_response.get("is_admin") is not True:
    raise SystemExit("expected second login response is_admin to be true after promotion")
PY
then
    append_log_line "${test_log_temp_file}" "admin promote validation failed"
    publish_failure_logs
    exit 1
fi

renew_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${renew_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${login_token}" \
        "${base_url}/api/auth/token/renew"
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
        "${base_url}/api/auth/logout"
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
        "${base_url}/api/auth/logout"
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
