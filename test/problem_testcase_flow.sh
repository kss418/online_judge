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

http_port="${PROBLEM_TESTCASE_FLOW_TEST_HTTP_PORT:-18083}"
base_url="${PROBLEM_TESTCASE_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${PROBLEM_TESTCASE_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${PROBLEM_TESTCASE_FLOW_TEST_LOGIN_ID:-problem_testcase_flow_test_$(date +%s)_$$}"
raw_password="${PROBLEM_TESTCASE_FLOW_TEST_PASSWORD:-password123}"
test_db_name="problem_testcase_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_problem_testcase_flow.log"
server_log_name="test_problem_testcase_flow_server.log"
test_log_temp_file="$(mktemp)"
server_log_temp_file="$(mktemp)"
sign_up_response_file="$(mktemp)"
create_problem_response_file="$(mktemp)"
create_testcase_response_file="$(mktemp)"
create_empty_testcase_response_file="$(mktemp)"
get_problem_response_file="$(mktemp)"

cleanup(){
    cleanup_http_server
    drop_test_database

    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${sign_up_response_file}" \
        "${create_problem_response_file}" \
        "${create_testcase_response_file}" \
        "${create_empty_testcase_response_file}" \
        "${get_problem_response_file}"

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

read -r sign_up_user_id sign_up_token < <(
    sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "problem testcase flow"
)
promote_admin_user "${sign_up_user_id}" "problem testcase flow" >/dev/null
problem_id="$(
    create_problem_via_api "${sign_up_token}" "${create_problem_response_file}" "problem testcase flow"
)"

testcase_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "testcase_input": "1 2\n",
            "testcase_output": "3\n",
        }
    )
)
PY
)"

create_testcase_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${create_testcase_response_file}" \
        --write-out "%{http_code}" \
        -X POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${testcase_request_body}" \
        "${base_url}/api/problem/${problem_id}/testcases"
)"

if [[ "${create_testcase_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "first testcase create failed: status=${create_testcase_status_code}"
    publish_failure_logs
    echo "problem testcase flow first create failed: expected status 201, got ${create_testcase_status_code}" >&2
    echo "response body:" >&2
    cat "${create_testcase_response_file}" >&2
    exit 1
fi

python3 - "${create_testcase_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

testcase_id = response.get("testcase_id")
testcase_order = response.get("testcase_order")
if not isinstance(testcase_id, int) or testcase_id <= 0:
    raise SystemExit("invalid testcase_id after first testcase create")
if testcase_order != 1:
    raise SystemExit("unexpected testcase_order after first testcase create")
PY

print_success_log "problem testcase first create success"

empty_testcase_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "testcase_input": "",
            "testcase_output": "",
        }
    )
)
PY
)"

create_empty_testcase_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${create_empty_testcase_response_file}" \
        --write-out "%{http_code}" \
        -X POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${empty_testcase_request_body}" \
        "${base_url}/api/problem/${problem_id}/testcases"
)"

if [[ "${create_empty_testcase_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "empty testcase create failed: status=${create_empty_testcase_status_code}"
    publish_failure_logs
    echo "problem testcase flow empty create failed: expected status 201, got ${create_empty_testcase_status_code}" >&2
    echo "response body:" >&2
    cat "${create_empty_testcase_response_file}" >&2
    exit 1
fi

python3 - "${create_empty_testcase_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

testcase_id = response.get("testcase_id")
testcase_order = response.get("testcase_order")
if not isinstance(testcase_id, int) or testcase_id <= 0:
    raise SystemExit("invalid testcase_id after empty testcase create")
if testcase_order != 2:
    raise SystemExit("unexpected testcase_order after empty testcase create")
PY

print_success_log "problem testcase empty create success"

get_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${get_problem_response_file}" \
        --write-out "%{http_code}" \
        "${base_url}/api/problem/${problem_id}"
)"

if [[ "${get_problem_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "get problem failed after testcase create: status=${get_problem_status_code}"
    publish_failure_logs
    echo "problem testcase flow verification failed: expected status 200, got ${get_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${get_problem_response_file}" >&2
    exit 1
fi

python3 - "${get_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after testcase create")
if response.get("version") != 3:
    raise SystemExit("version mismatch after testcase create")
if response.get("limits") != {"memory_limit_mb": 256, "time_limit_ms": 1000}:
    raise SystemExit("limits changed unexpectedly after testcase create")
if response.get("statement", "missing") is not None:
    raise SystemExit("expected null statement after testcase create")
if response.get("sample_count") != 0:
    raise SystemExit("sample_count changed unexpectedly after testcase create")
if response.get("samples") != []:
    raise SystemExit("samples changed unexpectedly after testcase create")
statistics = response.get("statistics")
if statistics != {"submission_count": 0, "accepted_count": 0}:
    raise SystemExit("statistics changed unexpectedly after testcase create")
if "testcase_count" in response:
    raise SystemExit("unexpected testcase_count field")
PY

append_log_line "${test_log_temp_file}" "problem testcase flow test passed"
print_success_log "problem testcase flow test passed: problem_id=${problem_id}, admin_user_id=${sign_up_user_id}"
