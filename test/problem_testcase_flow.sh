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
list_testcases_response_file="$(mktemp)"
update_testcase_response_file="$(mktemp)"
updated_testcases_response_file="$(mktemp)"
delete_testcase_response_file="$(mktemp)"
remaining_testcases_response_file="$(mktemp)"
get_problem_response_file="$(mktemp)"
updated_problem_response_file="$(mktemp)"
deleted_problem_response_file="$(mktemp)"

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
        "${list_testcases_response_file}" \
        "${update_testcase_response_file}" \
        "${updated_testcases_response_file}" \
        "${delete_testcase_response_file}" \
        "${remaining_testcases_response_file}" \
        "${get_problem_response_file}" \
        "${updated_problem_response_file}" \
        "${deleted_problem_response_file}"

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
    create_problem_via_api \
        "${sign_up_token}" \
        "${create_problem_response_file}" \
        "problem testcase flow" \
        "Problem Testcase Flow"
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
        "${base_url}/api/problem/${problem_id}/testcase"
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
        "${base_url}/api/problem/${problem_id}/testcase"
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

list_testcases_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${list_testcases_response_file}" \
        --write-out "%{http_code}" \
        -H "Authorization: Bearer ${sign_up_token}" \
        "${base_url}/api/problem/${problem_id}/testcase"
)"

if [[ "${list_testcases_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "list testcases failed: status=${list_testcases_status_code}"
    publish_failure_logs
    echo "problem testcase flow list failed: expected status 200, got ${list_testcases_status_code}" >&2
    echo "response body:" >&2
    cat "${list_testcases_response_file}" >&2
    exit 1
fi

python3 - "${list_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 2:
    raise SystemExit("unexpected testcase_count after list")

testcases = response.get("testcases")
if not isinstance(testcases, list) or len(testcases) != 2:
    raise SystemExit("unexpected testcase list size")

first_testcase = testcases[0]
second_testcase = testcases[1]

if first_testcase.get("testcase_order") != 1:
    raise SystemExit("unexpected first testcase_order after list")
if first_testcase.get("testcase_input") != "1 2\n":
    raise SystemExit("unexpected first testcase_input after list")
if first_testcase.get("testcase_output") != "3\n":
    raise SystemExit("unexpected first testcase_output after list")

if second_testcase.get("testcase_order") != 2:
    raise SystemExit("unexpected second testcase_order after list")
if second_testcase.get("testcase_input") != "":
    raise SystemExit("unexpected second testcase_input after list")
if second_testcase.get("testcase_output") != "":
    raise SystemExit("unexpected second testcase_output after list")
PY

print_success_log "problem testcase list success"

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
if response.get("title") != "Problem Testcase Flow":
    raise SystemExit("title mismatch after testcase create")
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

update_testcase_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "testcase_input": "10 20\n",
            "testcase_output": "30\n",
        }
    )
)
PY
)"

update_testcase_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${update_testcase_response_file}" \
        --write-out "%{http_code}" \
        -X PUT \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${update_testcase_request_body}" \
        "${base_url}/api/problem/${problem_id}/testcase/1"
)"

if [[ "${update_testcase_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "update testcase failed: status=${update_testcase_status_code}"
    publish_failure_logs
    echo "problem testcase flow update failed: expected status 200, got ${update_testcase_status_code}" >&2
    echo "response body:" >&2
    cat "${update_testcase_response_file}" >&2
    exit 1
fi

python3 - "${update_testcase_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_order") != 1:
    raise SystemExit("unexpected testcase_order after update")
if response.get("testcase_input") != "10 20\n":
    raise SystemExit("unexpected testcase_input after update")
if response.get("testcase_output") != "30\n":
    raise SystemExit("unexpected testcase_output after update")
testcase_id = response.get("testcase_id")
if not isinstance(testcase_id, int) or testcase_id <= 0:
    raise SystemExit("invalid testcase_id after update")
PY

print_success_log "problem testcase update success"

updated_testcases_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${updated_testcases_response_file}" \
        --write-out "%{http_code}" \
        -H "Authorization: Bearer ${sign_up_token}" \
        "${base_url}/api/problem/${problem_id}/testcase"
)"

if [[ "${updated_testcases_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "list updated testcases failed: status=${updated_testcases_status_code}"
    publish_failure_logs
    echo "problem testcase flow updated list failed: expected status 200, got ${updated_testcases_status_code}" >&2
    echo "response body:" >&2
    cat "${updated_testcases_response_file}" >&2
    exit 1
fi

python3 - "${updated_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 2:
    raise SystemExit("unexpected testcase_count after update")

testcases = response.get("testcases")
if not isinstance(testcases, list) or len(testcases) != 2:
    raise SystemExit("unexpected testcase list size after update")

first_testcase = testcases[0]
if first_testcase.get("testcase_order") != 1:
    raise SystemExit("unexpected first testcase_order after update")
if first_testcase.get("testcase_input") != "10 20\n":
    raise SystemExit("unexpected first testcase_input after update")
if first_testcase.get("testcase_output") != "30\n":
    raise SystemExit("unexpected first testcase_output after update")
PY

updated_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${updated_problem_response_file}" \
        --write-out "%{http_code}" \
        "${base_url}/api/problem/${problem_id}"
)"

if [[ "${updated_problem_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "get problem failed after testcase update: status=${updated_problem_status_code}"
    publish_failure_logs
    echo "problem testcase flow update verification failed: expected status 200, got ${updated_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${updated_problem_response_file}" >&2
    exit 1
fi

python3 - "${updated_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after testcase update")
if response.get("version") != 4:
    raise SystemExit("version mismatch after testcase update")
PY

delete_testcase_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${delete_testcase_response_file}" \
        --write-out "%{http_code}" \
        -X DELETE \
        -H "Authorization: Bearer ${sign_up_token}" \
        "${base_url}/api/problem/${problem_id}/testcase"
)"

if [[ "${delete_testcase_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "delete testcase failed: status=${delete_testcase_status_code}"
    publish_failure_logs
    echo "problem testcase flow delete failed: expected status 200, got ${delete_testcase_status_code}" >&2
    echo "response body:" >&2
    cat "${delete_testcase_response_file}" >&2
    exit 1
fi

if ! python3 - "${delete_testcase_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("message") != "problem testcase deleted":
    raise SystemExit("unexpected delete testcase message")
PY
then
    append_log_line "${test_log_temp_file}" "delete testcase body mismatch"
    publish_failure_logs
    echo "problem testcase flow delete failed: unexpected response body" >&2
    cat "${delete_testcase_response_file}" >&2
    exit 1
fi

print_success_log "problem testcase delete success"

remaining_testcases_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${remaining_testcases_response_file}" \
        --write-out "%{http_code}" \
        -H "Authorization: Bearer ${sign_up_token}" \
        "${base_url}/api/problem/${problem_id}/testcase"
)"

if [[ "${remaining_testcases_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "list remaining testcases failed: status=${remaining_testcases_status_code}"
    publish_failure_logs
    echo "problem testcase flow remaining list failed: expected status 200, got ${remaining_testcases_status_code}" >&2
    echo "response body:" >&2
    cat "${remaining_testcases_response_file}" >&2
    exit 1
fi

python3 - "${remaining_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 1:
    raise SystemExit("unexpected testcase_count after delete")

testcases = response.get("testcases")
if not isinstance(testcases, list) or len(testcases) != 1:
    raise SystemExit("unexpected testcase list size after delete")

first_testcase = testcases[0]
if first_testcase.get("testcase_order") != 1:
    raise SystemExit("unexpected remaining testcase_order after delete")
if first_testcase.get("testcase_input") != "10 20\n":
    raise SystemExit("unexpected remaining testcase_input after delete")
if first_testcase.get("testcase_output") != "30\n":
    raise SystemExit("unexpected remaining testcase_output after delete")
PY

deleted_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${deleted_problem_response_file}" \
        --write-out "%{http_code}" \
        "${base_url}/api/problem/${problem_id}"
)"

if [[ "${deleted_problem_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "get problem failed after testcase delete: status=${deleted_problem_status_code}"
    publish_failure_logs
    echo "problem testcase flow delete verification failed: expected status 200, got ${deleted_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${deleted_problem_response_file}" >&2
    exit 1
fi

python3 - "${deleted_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after testcase delete")
if response.get("version") != 5:
    raise SystemExit("version mismatch after testcase delete")
PY

append_log_line "${test_log_temp_file}" "problem testcase flow test passed"
print_success_log "problem testcase flow test passed: problem_id=${problem_id}, admin_user_id=${sign_up_user_id}"
