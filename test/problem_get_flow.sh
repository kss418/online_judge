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

http_port="${PROBLEM_GET_FLOW_TEST_HTTP_PORT:-18081}"
base_url="${PROBLEM_GET_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${PROBLEM_GET_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
test_db_name="problem_get_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_problem_get_flow.log"
server_log_name="test_problem_get_flow_server.log"
init_flow_test
register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file full_problem_response_file
register_temp_file blank_problem_response_file
register_temp_file missing_problem_response_file

create_full_problem(){
    require_db_env

    PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v ON_ERROR_STOP=1 \
        -qAt <<'SQL' | sed -n '1p'
WITH created_problem AS (
    INSERT INTO problems(version, title)
    VALUES(3, 'A + B')
    RETURNING problem_id
), inserted_limits AS (
    INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at)
    SELECT problem_id, 512, 2000, NOW()
    FROM created_problem
), inserted_statistics AS (
    INSERT INTO problem_statistics(problem_id, submission_count, accepted_count, updated_at)
    SELECT problem_id, 12, 7, NOW()
    FROM created_problem
), inserted_statement AS (
    INSERT INTO problem_statements(
        problem_id,
        description,
        input_format,
        output_format,
        sample_count,
        testcase_count,
        note,
        created_at,
        updated_at
    )
    SELECT
        problem_id,
        'Print A+B.',
        'Two integers A and B are given.',
        'Print A+B.',
        1,
        2,
        '1 <= A, B <= 10',
        NOW(),
        NOW()
    FROM created_problem
), inserted_sample AS (
    INSERT INTO problem_samples(problem_id, sample_order, sample_input, sample_output)
    SELECT problem_id, 1, '1 2', '3'
    FROM created_problem
), inserted_testcase_1 AS (
    INSERT INTO problem_testcases(problem_id, testcase_order, testcase_input, testcase_output)
    SELECT problem_id, 1, '1 2', '3'
    FROM created_problem
), inserted_testcase_2 AS (
    INSERT INTO problem_testcases(problem_id, testcase_order, testcase_input, testcase_output)
    SELECT problem_id, 2, '10 20', '30'
    FROM created_problem
)
SELECT problem_id
FROM created_problem;
SQL
}

create_blank_problem(){
    require_db_env

    PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v ON_ERROR_STOP=1 \
        -qAt <<'SQL' | sed -n '1p'
WITH created_problem AS (
    INSERT INTO problems(version, title)
    VALUES(1, 'Blank Problem')
    RETURNING problem_id
), inserted_limits AS (
    INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at)
    SELECT problem_id, 256, 1000, NOW()
    FROM created_problem
), inserted_statistics AS (
    INSERT INTO problem_statistics(problem_id, submission_count, accepted_count, updated_at)
    SELECT problem_id, 0, 0, NOW()
    FROM created_problem
)
SELECT problem_id
FROM created_problem;
SQL
}

trap 'finish_flow_test cleanup_http_server drop_test_database' EXIT

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

full_problem_id="$(create_full_problem)"
blank_problem_id="$(create_blank_problem)"
missing_problem_id=$((blank_problem_id + 999999))

append_log_line "${test_log_temp_file}" "full_problem_id=${full_problem_id}"
append_log_line "${test_log_temp_file}" "blank_problem_id=${blank_problem_id}"
append_log_line "${test_log_temp_file}" "missing_problem_id=${missing_problem_id}"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${full_problem_id}" \
    "${full_problem_response_file}" \
    "200" \
    "full problem get"

if ! python3 - "${full_problem_response_file}" "${full_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
expected_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch")

if response.get("title") != "A + B":
    raise SystemExit("title mismatch")

if response.get("version") != 3:
    raise SystemExit("version mismatch")

limits = response.get("limits")
if limits != {"memory_limit_mb": 512, "time_limit_ms": 2000}:
    raise SystemExit("limits mismatch")

statement = response.get("statement")
expected_statement = {
    "description": "Print A+B.",
    "input_format": "Two integers A and B are given.",
    "output_format": "Print A+B.",
    "note": "1 <= A, B <= 10",
}
if statement != expected_statement:
    raise SystemExit("statement mismatch")

if response.get("sample_count") != 1:
    raise SystemExit("sample_count mismatch")

if "testcase_count" in response:
    raise SystemExit("unexpected testcase_count field")

samples = response.get("samples")
expected_samples = [
    {
        "sample_order": 1,
        "sample_input": "1 2",
        "sample_output": "3",
    }
]
if samples != expected_samples:
    raise SystemExit("samples mismatch")

statistics = response.get("statistics")
expected_statistics = {
    "submission_count": 12,
    "accepted_count": 7,
}
if statistics != expected_statistics:
    raise SystemExit("statistics mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "full problem response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "problem detail response validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${blank_problem_id}" \
    "${blank_problem_response_file}" \
    "200" \
    "blank problem get"

if ! python3 - "${blank_problem_response_file}" "${blank_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
expected_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_id") != expected_problem_id:
    raise SystemExit("blank problem_id mismatch")

if response.get("title") != "Blank Problem":
    raise SystemExit("blank title mismatch")

if response.get("version") != 1:
    raise SystemExit("blank version mismatch")

if response.get("limits") != {"memory_limit_mb": 256, "time_limit_ms": 1000}:
    raise SystemExit("blank limits mismatch")

if response.get("statement", "missing") is not None:
    raise SystemExit("expected null statement")

if response.get("sample_count") != 0:
    raise SystemExit("blank sample_count mismatch")

if "testcase_count" in response:
    raise SystemExit("unexpected blank testcase_count field")

if response.get("samples") != []:
    raise SystemExit("expected empty samples")

if response.get("statistics") != {"submission_count": 0, "accepted_count": 0}:
    raise SystemExit("blank statistics mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "blank problem response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "blank problem response validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${missing_problem_id}" \
    "${missing_problem_response_file}" \
    "404" \
    "missing problem get"
assert_json_error_code \
    "${missing_problem_response_file}" \
    "problem_not_found" \
    "missing problem get"
assert_json_error_message \
    "${missing_problem_response_file}" \
    "problem not found" \
    "missing problem get"

append_log_line "${test_log_temp_file}" "problem get flow test passed"
print_success_log \
    "problem get flow test passed: full_problem_id=${full_problem_id}, blank_problem_id=${blank_problem_id}"
