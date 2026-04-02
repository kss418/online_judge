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

http_port="${PROBLEM_STATEMENT_FLOW_TEST_HTTP_PORT:-18082}"
base_url="${PROBLEM_STATEMENT_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${PROBLEM_STATEMENT_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${PROBLEM_STATEMENT_FLOW_TEST_LOGIN_ID:-$(make_test_login_id st)}"
raw_password="${PROBLEM_STATEMENT_FLOW_TEST_PASSWORD:-password123}"
test_db_name="problem_statement_flow_test_$$_$(date +%s)"
test_database_created="0"
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
missing_statement_response_file="$(mktemp)"
missing_limits_response_file="$(mktemp)"

cleanup(){
    cleanup_http_server
    drop_test_database

    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${sign_up_response_file}" \
        "${create_problem_response_file}" \
        "${set_statement_response_file}" \
        "${get_problem_response_file}" \
        "${clear_statement_response_file}" \
        "${updated_problem_response_file}" \
        "${missing_statement_response_file}" \
        "${missing_limits_response_file}"

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
    sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "problem statement flow"
)
promote_admin_user "${sign_up_user_id}" "problem statement flow" >/dev/null
problem_id="$(
    create_problem_via_api \
        "${sign_up_token}" \
        "${create_problem_response_file}" \
        "problem statement flow" \
        "Problem Statement Flow"
)"
missing_problem_id=$((problem_id + 999999))

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

limits_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "memory_limit_mb": 512,
            "time_limit_ms": 2000,
        }
    )
)
PY
)"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/problem/${missing_problem_id}/statement" \
    "${missing_statement_response_file}" \
    "404" \
    "missing problem statement update" \
    "${sign_up_token}" \
    "${statement_request_body}"
assert_json_error_code \
    "${missing_statement_response_file}" \
    "not_found" \
    "missing problem statement update"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/problem/${missing_problem_id}/limits" \
    "${missing_limits_response_file}" \
    "404" \
    "missing problem limits update" \
    "${sign_up_token}" \
    "${limits_request_body}"
assert_json_error_code \
    "${missing_limits_response_file}" \
    "not_found" \
    "missing problem limits update"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/problem/${problem_id}/statement" \
    "${set_statement_response_file}" \
    "200" \
    "set statement" \
    "${sign_up_token}" \
    "${statement_request_body}"
assert_json_message \
    "${set_statement_response_file}" \
    "problem statement updated" \
    "set statement"

print_success_log "problem statement initial update success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${get_problem_response_file}" \
    "200" \
    "get problem after statement update"

if ! python3 - "${get_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
expected_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch after first statement update")

if response.get("title") != "Problem Statement Flow":
    raise SystemExit("title mismatch after first statement update")

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

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/problem/${problem_id}/statement" \
    "${clear_statement_response_file}" \
    "200" \
    "clear statement" \
    "${sign_up_token}" \
    "${clear_statement_request_body}"
assert_json_message \
    "${clear_statement_response_file}" \
    "problem statement updated" \
    "clear statement"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${updated_problem_response_file}" \
    "200" \
    "get problem after clear statement"

if ! python3 - "${updated_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
expected_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch after second statement update")

if response.get("title") != "Problem Statement Flow":
    raise SystemExit("title mismatch after second statement update")

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
