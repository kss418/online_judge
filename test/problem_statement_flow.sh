#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/util.sh"
# shellcheck disable=SC1091
source "${script_dir}/http_server_util.sh"

if [[ -f "${project_root}/.env" ]]; then
    set -a
    # shellcheck disable=SC1091
    source "${project_root}/.env"
    set +a
fi

http_port="${HTTP_PORT:-18080}"
base_url="${PROBLEM_STATEMENT_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${PROBLEM_STATEMENT_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${PROBLEM_STATEMENT_FLOW_TEST_LOGIN_ID:-problem_statement_flow_test_$(date +%s)_$$}"
raw_password="${PROBLEM_STATEMENT_FLOW_TEST_PASSWORD:-password123}"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_problem_statement_flow.log"
server_log_name="test_problem_statement_flow_server.log"
test_log_temp_file="$(mktemp)"
server_log_temp_file="$(mktemp)"
sign_up_response_file="$(mktemp)"
create_problem_response_file="$(mktemp)"
set_statement_response_file="$(mktemp)"
get_problem_response_file="$(mktemp)"
clear_statement_response_file="$(mktemp)"
updated_problem_response_file="$(mktemp)"

cleanup(){
    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${sign_up_response_file}" \
        "${create_problem_response_file}" \
        "${set_statement_response_file}" \
        "${get_problem_response_file}" \
        "${clear_statement_response_file}" \
        "${updated_problem_response_file}"

    cleanup_http_server
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

require_db_env(){
    if [[ -z "${DB_HOST:-}" || -z "${DB_PORT:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWORD:-}" || -z "${DB_NAME:-}" ]]; then
        echo "missing required db envs" >&2
        exit 1
    fi
}

promote_admin(){
    local user_id="$1"

    require_db_env

    if [[ -z "${user_id}" ]]; then
        echo "missing user_id" >&2
        return 1
    fi

    PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v ON_ERROR_STOP=1 \
        -qAt <<SQL | sed -n '1p'
UPDATE users
SET is_admin = TRUE, updated_at = NOW()
WHERE user_id = ${user_id}
RETURNING user_id;
SQL
}

trap cleanup EXIT

require_command curl
require_command psql
require_command python3

append_log_line "${test_log_temp_file}" "base_url=${base_url}"
append_log_line "${test_log_temp_file}" "user_login_id=${user_login_id}"

ensure_http_server

sign_up_request_body="$(
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
        -d "${sign_up_request_body}" \
        "${base_url}/api/auth/sign-up"
)"

if [[ "${sign_up_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "sign-up failed: status=${sign_up_status_code}"
    publish_failure_logs
    echo "problem statement flow sign-up failed: expected status 201, got ${sign_up_status_code}" >&2
    echo "response body:" >&2
    cat "${sign_up_response_file}" >&2
    exit 1
fi

sign_up_user_id="$(
    python3 - "${sign_up_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

user_id = response.get("user_id")
if not isinstance(user_id, int) or user_id <= 0:
    raise SystemExit("invalid user_id in sign-up response")

print(user_id)
PY
)"

sign_up_token="$(
    python3 - "${sign_up_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

token = response.get("token")
if not isinstance(token, str) or not token:
    raise SystemExit("invalid token in sign-up response")

print(token)
PY
)"

promoted_user_id="$(promote_admin "${sign_up_user_id}")"
if [[ "${promoted_user_id}" != "${sign_up_user_id}" ]]; then
    append_log_line "${test_log_temp_file}" "admin promotion failed: user_id=${sign_up_user_id}"
    publish_failure_logs
    echo "problem statement flow admin promotion failed" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "admin promotion succeeded: user_id=${sign_up_user_id}"

create_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${create_problem_response_file}" \
        --write-out "%{http_code}" \
        -X POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        "${base_url}/api/problem"
)"

if [[ "${create_problem_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "problem create failed: status=${create_problem_status_code}"
    publish_failure_logs
    echo "problem statement flow create problem failed: expected status 201, got ${create_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${create_problem_response_file}" >&2
    exit 1
fi

problem_id="$(
    python3 - "${create_problem_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = response.get("problem_id")
if not isinstance(problem_id, int) or problem_id <= 0:
    raise SystemExit("invalid problem_id in create problem response")

print(problem_id)
PY
)"

append_log_line "${test_log_temp_file}" "problem created: problem_id=${problem_id}"

statement_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "description": "Print A+B.",
            "input_format": "Two integers A and B are given.",
            "output_format": "Print A+B.",
            "note": "1 <= A, B <= 10",
        }
    )
)
PY
)"

set_statement_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${set_statement_response_file}" \
        --write-out "%{http_code}" \
        -X PUT \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${statement_request_body}" \
        "${base_url}/api/problem/${problem_id}/statement"
)"

if [[ "${set_statement_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "set statement failed: status=${set_statement_status_code}"
    publish_failure_logs
    echo "problem statement update failed: expected status 200, got ${set_statement_status_code}" >&2
    echo "response body:" >&2
    cat "${set_statement_response_file}" >&2
    exit 1
fi

if [[ "$(cat "${set_statement_response_file}")" != "problem statement updated" ]]; then
    append_log_line "${test_log_temp_file}" "set statement body mismatch"
    publish_failure_logs
    echo "problem statement update failed: unexpected response body" >&2
    cat "${set_statement_response_file}" >&2
    exit 1
fi

print_success_log "problem statement initial update success"

get_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${get_problem_response_file}" \
        --write-out "%{http_code}" \
        "${base_url}/api/problem/${problem_id}"
)"

if [[ "${get_problem_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "get problem after statement update failed: status=${get_problem_status_code}"
    publish_failure_logs
    echo "problem statement verification failed: expected status 200, got ${get_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${get_problem_response_file}" >&2
    exit 1
fi

if ! python3 - "${get_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
expected_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch after first statement update")

if response.get("version") != 2:
    raise SystemExit("version mismatch after first statement update")

if response.get("limits") != {"memory_limit_mb": 256, "time_limit_ms": 1000}:
    raise SystemExit("limits changed unexpectedly after first statement update")

expected_statement = {
    "description": "Print A+B.",
    "input_format": "Two integers A and B are given.",
    "output_format": "Print A+B.",
    "note": "1 <= A, B <= 10",
}
if response.get("statement") != expected_statement:
    raise SystemExit("statement mismatch after first statement update")

if response.get("sample_count") != 0:
    raise SystemExit("sample_count changed unexpectedly after first statement update")

if response.get("samples") != []:
    raise SystemExit("samples changed unexpectedly after first statement update")

if response.get("statistics") != {"submission_count": 0, "accepted_count": 0}:
    raise SystemExit("statistics changed unexpectedly after first statement update")
PY
then
    append_log_line "${test_log_temp_file}" "first statement verification failed"
    publish_failure_logs
    exit 1
fi

clear_statement_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "description": "Print the sum of two integers.",
            "input_format": "A single line contains two integers.",
            "output_format": "Output the sum on one line.",
            "note": "",
        }
    )
)
PY
)"

clear_statement_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${clear_statement_response_file}" \
        --write-out "%{http_code}" \
        -X PUT \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${clear_statement_request_body}" \
        "${base_url}/api/problem/${problem_id}/statement"
)"

if [[ "${clear_statement_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "clear statement failed: status=${clear_statement_status_code}"
    publish_failure_logs
    echo "problem statement clear failed: expected status 200, got ${clear_statement_status_code}" >&2
    echo "response body:" >&2
    cat "${clear_statement_response_file}" >&2
    exit 1
fi

if [[ "$(cat "${clear_statement_response_file}")" != "problem statement updated" ]]; then
    append_log_line "${test_log_temp_file}" "clear statement body mismatch"
    publish_failure_logs
    echo "problem statement clear failed: unexpected response body" >&2
    cat "${clear_statement_response_file}" >&2
    exit 1
fi

updated_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${updated_problem_response_file}" \
        --write-out "%{http_code}" \
        "${base_url}/api/problem/${problem_id}"
)"

if [[ "${updated_problem_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "get problem after clear statement failed: status=${updated_problem_status_code}"
    publish_failure_logs
    echo "problem statement clear verification failed: expected status 200, got ${updated_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${updated_problem_response_file}" >&2
    exit 1
fi

if ! python3 - "${updated_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
expected_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch after second statement update")

if response.get("version") != 3:
    raise SystemExit("version mismatch after second statement update")

if response.get("limits") != {"memory_limit_mb": 256, "time_limit_ms": 1000}:
    raise SystemExit("limits changed unexpectedly after second statement update")

statement = response.get("statement")
expected_statement = {
    "description": "Print the sum of two integers.",
    "input_format": "A single line contains two integers.",
    "output_format": "Output the sum on one line.",
}
if statement != expected_statement:
    raise SystemExit("statement mismatch after second statement update")

if response.get("sample_count") != 0:
    raise SystemExit("sample_count changed unexpectedly after second statement update")

if response.get("samples") != []:
    raise SystemExit("samples changed unexpectedly after second statement update")

if response.get("statistics") != {"submission_count": 0, "accepted_count": 0}:
    raise SystemExit("statistics changed unexpectedly after second statement update")
PY
then
    append_log_line "${test_log_temp_file}" "second statement verification failed"
    publish_failure_logs
    exit 1
fi

append_log_line "${test_log_temp_file}" "problem statement flow test passed"
print_success_log "problem statement flow test passed: problem_id=${problem_id}, admin_user_id=${sign_up_user_id}"
