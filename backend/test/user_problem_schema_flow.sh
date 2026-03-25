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

if [[ -f "${project_root}/.env" ]]; then
    set -a
    # shellcheck disable=SC1091
    source "${project_root}/.env"
    set +a
fi

test_db_name="user_problem_schema_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
test_log_name="test_user_problem_schema_flow.log"

publish_failure_logs(){
    if [[ -z "${test_log_path}" ]]; then
        test_log_path="$(publish_log_file "${test_log_temp_file}" "${test_log_name}")"
        print_log_file_created "${test_log_path}"
    fi
}

init_flow_test
register_temp_file test_log_temp_file
register_temp_file user_problem_list_response_file
register_temp_file user_wrong_problem_list_response_file

trap 'finish_flow_test drop_test_database' EXIT

require_command psql
require_command python3

export DB_ADMIN_USER="${DB_ADMIN_USER:-${DB_USER:-postgres}}"
export DB_ADMIN_PASSWORD="${DB_ADMIN_PASSWORD:-${DB_PASSWORD:-postgres}}"
test_database_url="postgresql://${DB_USER}:${DB_PASSWORD}@${DB_HOST}:${DB_PORT}/${test_db_name}"
create_test_database
export DB_NAME="${test_db_name}"

append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"

apply_test_database_migrations

if ! PGPASSWORD="${DB_PASSWORD}" psql \
    -X \
    -h "${DB_HOST}" \
    -p "${DB_PORT}" \
    -U "${DB_USER}" \
    -d "${DB_NAME}" \
    -v ON_ERROR_STOP=1 <<'SQL' >>"${test_log_temp_file}" 2>&1
INSERT INTO users(user_name, user_login_id, user_password_hash)
VALUES
    ('alice', 'alice', 'hash_alice'),
    ('bob', 'bob', 'hash_bob');

INSERT INTO problems(title, version)
VALUES
    ('A + B', 1),
    ('Two Sum', 3);

INSERT INTO submissions(
    user_id,
    problem_id,
    language,
    source_code,
    status,
    created_at,
    updated_at
)
VALUES
    (1, 1, 'cpp17', 'accepted code', 'accepted', '2026-01-01T00:00:01Z', '2026-01-01T00:00:01Z'),
    (1, 2, 'cpp17', 'wrong code', 'wrong_answer', '2026-01-01T00:00:02Z', '2026-01-01T00:00:02Z'),
    (1, 2, 'cpp17', 'compile fail', 'compile_error', '2026-01-01T00:00:03Z', '2026-01-01T00:00:03Z'),
    (2, 1, 'python3', 'queued code', 'queued', '2026-01-01T00:00:04Z', '2026-01-01T00:00:04Z');
SQL
then
    append_log_line "${test_log_temp_file}" "fixture insert failed"
    publish_failure_logs
    exit 1
fi

if ! PGPASSWORD="${DB_PASSWORD}" psql \
    -X \
    -qAt \
    -h "${DB_HOST}" \
    -p "${DB_PORT}" \
    -U "${DB_USER}" \
    -d "${DB_NAME}" \
    -v ON_ERROR_STOP=1 <<'SQL' >"${user_problem_list_response_file}" 2>>"${test_log_temp_file}"
SELECT COALESCE(
    json_agg(
        row_to_json(row_value)
        ORDER BY row_value.user_id, row_value.problem_id
    )::text,
    '[]'
)
FROM (
    SELECT
        user_id,
        user_name,
        problem_id,
        problem_title,
        problem_version,
        submission_count,
        accepted_submission_count,
        failed_submission_count,
        problem_state
    FROM user_problem_list
) row_value;
SQL
then
    append_log_line "${test_log_temp_file}" "user_problem_list query failed"
    publish_failure_logs
    exit 1
fi

if ! python3 - "${user_problem_list_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    rows = json.load(response_file)

expected_rows = [
    {
        "user_id": 1,
        "user_name": "alice",
        "problem_id": 1,
        "problem_title": "A + B",
        "problem_version": 1,
        "submission_count": 1,
        "accepted_submission_count": 1,
        "failed_submission_count": 0,
        "problem_state": "solved",
    },
    {
        "user_id": 1,
        "user_name": "alice",
        "problem_id": 2,
        "problem_title": "Two Sum",
        "problem_version": 3,
        "submission_count": 2,
        "accepted_submission_count": 0,
        "failed_submission_count": 2,
        "problem_state": "wrong",
    },
    {
        "user_id": 2,
        "user_name": "bob",
        "problem_id": 1,
        "problem_title": "A + B",
        "problem_version": 1,
        "submission_count": 1,
        "accepted_submission_count": 0,
        "failed_submission_count": 0,
        "problem_state": "unattempted",
    },
    {
        "user_id": 2,
        "user_name": "bob",
        "problem_id": 2,
        "problem_title": "Two Sum",
        "problem_version": 3,
        "submission_count": 0,
        "accepted_submission_count": 0,
        "failed_submission_count": 0,
        "problem_state": "unattempted",
    },
]

if rows != expected_rows:
    raise SystemExit(f"user_problem_list mismatch: {rows!r}")
PY
then
    append_log_line "${test_log_temp_file}" "user_problem_list validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "user_problem_list view validated"

if ! PGPASSWORD="${DB_PASSWORD}" psql \
    -X \
    -qAt \
    -h "${DB_HOST}" \
    -p "${DB_PORT}" \
    -U "${DB_USER}" \
    -d "${DB_NAME}" \
    -v ON_ERROR_STOP=1 <<'SQL' >"${user_wrong_problem_list_response_file}" 2>>"${test_log_temp_file}"
SELECT COALESCE(
    json_agg(
        row_to_json(row_value)
        ORDER BY row_value.user_id, row_value.problem_id
    )::text,
    '[]'
)
FROM (
    SELECT
        user_id,
        user_name,
        problem_id,
        problem_title,
        problem_version,
        submission_count,
        accepted_submission_count,
        failed_submission_count,
        problem_state
    FROM user_wrong_problem_list
) row_value;
SQL
then
    append_log_line "${test_log_temp_file}" "user_wrong_problem_list query failed"
    publish_failure_logs
    exit 1
fi

if ! python3 - "${user_wrong_problem_list_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    rows = json.load(response_file)

expected_rows = [
    {
        "user_id": 1,
        "user_name": "alice",
        "problem_id": 2,
        "problem_title": "Two Sum",
        "problem_version": 3,
        "submission_count": 2,
        "accepted_submission_count": 0,
        "failed_submission_count": 2,
        "problem_state": "wrong",
    },
]

if rows != expected_rows:
    raise SystemExit(f"user_wrong_problem_list mismatch: {rows!r}")
PY
then
    append_log_line "${test_log_temp_file}" "user_wrong_problem_list validation failed"
    publish_failure_logs
    exit 1
fi

append_log_line "${test_log_temp_file}" "user_problem_schema flow test passed"
print_success_log \
    "user_problem_schema flow test passed: test_db_name=${test_db_name}"
