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

http_port="${PROBLEM_LIST_FLOW_TEST_HTTP_PORT:-18086}"
base_url="${PROBLEM_LIST_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${PROBLEM_LIST_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
test_db_name="problem_list_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_problem_list_flow.log"
server_log_name="test_problem_list_flow_server.log"
test_log_temp_file="$(mktemp)"
server_log_temp_file="$(mktemp)"
list_problem_response_file="$(mktemp)"
filtered_problem_response_file="$(mktemp)"
missing_problem_response_file="$(mktemp)"
invalid_query_response_file="$(mktemp)"

cleanup(){
    cleanup_http_server
    drop_test_database

    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${list_problem_response_file}" \
        "${filtered_problem_response_file}" \
        "${missing_problem_response_file}" \
        "${invalid_query_response_file}"

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
append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"

apply_test_database_migrations
ensure_dedicated_http_server

first_problem_id="$(create_problem_in_db "problem list flow" "A Plus B")"
second_problem_id="$(create_problem_in_db "problem list flow" "plus one")"
third_problem_id="$(create_problem_in_db "problem list flow" "Multiply")"

append_log_line "${test_log_temp_file}" "first_problem_id=${first_problem_id}"
append_log_line "${test_log_temp_file}" "second_problem_id=${second_problem_id}"
append_log_line "${test_log_temp_file}" "third_problem_id=${third_problem_id}"

list_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${list_problem_response_file}" \
        --write-out "%{http_code}" \
        "${base_url}/api/problem"
)"

if [[ "${list_problem_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "problem list failed: status=${list_problem_status_code}"
    publish_failure_logs
    echo "problem list flow failed: expected status 200, got ${list_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${list_problem_response_file}" >&2
    exit 1
fi

if ! python3 - "${list_problem_response_file}" "${first_problem_id}" "${second_problem_id}" "${third_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
first_problem_id = int(sys.argv[2])
second_problem_id = int(sys.argv[3])
third_problem_id = int(sys.argv[4])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_count") != 3:
    raise SystemExit("problem_count mismatch")

expected_problems = [
    {
        "problem_id": third_problem_id,
        "title": "Multiply",
        "version": 1,
    },
    {
        "problem_id": second_problem_id,
        "title": "plus one",
        "version": 1,
    },
    {
        "problem_id": first_problem_id,
        "title": "A Plus B",
        "version": 1,
    },
]
if response.get("problems") != expected_problems:
    raise SystemExit("problem list mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "problem list response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "problem list response validated"

filtered_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${filtered_problem_response_file}" \
        --write-out "%{http_code}" \
        "${base_url}/api/problem?title=PLUS"
)"

if [[ "${filtered_problem_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "filtered problem list failed: status=${filtered_problem_status_code}"
    publish_failure_logs
    echo "filtered problem list flow failed: expected status 200, got ${filtered_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${filtered_problem_response_file}" >&2
    exit 1
fi

if ! python3 - "${filtered_problem_response_file}" "${first_problem_id}" "${second_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
first_problem_id = int(sys.argv[2])
second_problem_id = int(sys.argv[3])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_count") != 2:
    raise SystemExit("filtered problem_count mismatch")

expected_problems = [
    {
        "problem_id": second_problem_id,
        "title": "plus one",
        "version": 1,
    },
    {
        "problem_id": first_problem_id,
        "title": "A Plus B",
        "version": 1,
    },
]
if response.get("problems") != expected_problems:
    raise SystemExit("filtered problem list mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "filtered problem list response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "problem list title filter validated"

missing_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${missing_problem_response_file}" \
        --write-out "%{http_code}" \
        "${base_url}/api/problem?title=divide"
)"

if [[ "${missing_problem_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "missing title problem list failed: status=${missing_problem_status_code}"
    publish_failure_logs
    echo "missing title problem list flow failed: expected status 200, got ${missing_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${missing_problem_response_file}" >&2
    exit 1
fi

if ! python3 - "${missing_problem_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_count") != 0:
    raise SystemExit("missing title problem_count mismatch")
if response.get("problems") != []:
    raise SystemExit("missing title problems mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "missing title problem list response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "problem list empty filter validated"

invalid_query_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${invalid_query_response_file}" \
        --write-out "%{http_code}" \
        "${base_url}/api/problem?unsupported=value"
)"

if [[ "${invalid_query_status_code}" != "400" ]]; then
    append_log_line "${test_log_temp_file}" "invalid query problem list failed: status=${invalid_query_status_code}"
    publish_failure_logs
    echo "invalid query problem list flow failed: expected status 400, got ${invalid_query_status_code}" >&2
    echo "response body:" >&2
    cat "${invalid_query_response_file}" >&2
    exit 1
fi

if [[ "$(cat "${invalid_query_response_file}")" != "unsupported query parameter: unsupported" ]]; then
    append_log_line "${test_log_temp_file}" "invalid query body mismatch"
    publish_failure_logs
    echo "invalid query problem list flow failed: unexpected response body" >&2
    cat "${invalid_query_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "problem list flow test passed"
print_success_log \
    "problem list flow test passed: first_problem_id=${first_problem_id}, second_problem_id=${second_problem_id}, third_problem_id=${third_problem_id}"
