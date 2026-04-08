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

http_port="${PROBLEM_LIST_FLOW_TEST_HTTP_PORT:-18086}"
base_url="${PROBLEM_LIST_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${PROBLEM_LIST_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
test_db_name="problem_list_flow_test_$$_$(date +%s)"
test_database_created="0"
user_login_id="${PROBLEM_LIST_FLOW_TEST_LOGIN_ID:-$(make_test_login_id pl)}"
raw_password="${PROBLEM_LIST_FLOW_TEST_PASSWORD:-password123!}"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_problem_list_flow.log"
server_log_name="test_problem_list_flow_server.log"

init_flow_test
register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file sign_up_response_file
register_temp_file list_problem_response_file
register_temp_file authenticated_list_problem_response_file
register_temp_file filtered_problem_response_file
register_temp_file missing_problem_response_file
register_temp_file filtered_problem_id_response_file
register_temp_file solved_problem_response_file
register_temp_file sorted_limited_problem_response_file
register_temp_file offset_problem_response_file
register_temp_file invalid_state_problem_response_file
register_temp_file invalid_query_response_file
register_temp_file invalid_token_response_file

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
append_log_line "${test_log_temp_file}" "user_login_id=${user_login_id}"

apply_test_database_migrations
ensure_dedicated_http_server

first_problem_id="$(create_problem_in_db "problem list flow" "A Plus B")"
second_problem_id="$(create_problem_in_db "problem list flow" "plus one")"
third_problem_id="$(create_problem_in_db "problem list flow" "Multiply")"
fourth_problem_id="$(create_problem_in_db "problem list flow" "Queue Later")"

append_log_line "${test_log_temp_file}" "first_problem_id=${first_problem_id}"
append_log_line "${test_log_temp_file}" "second_problem_id=${second_problem_id}"
append_log_line "${test_log_temp_file}" "third_problem_id=${third_problem_id}"
append_log_line "${test_log_temp_file}" "fourth_problem_id=${fourth_problem_id}"

read -r sign_up_user_id sign_up_token <<EOF
$(sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "problem list flow")
EOF

append_log_line "${test_log_temp_file}" "sign_up_user_id=${sign_up_user_id}"

if ! PGPASSWORD="${DB_PASSWORD}" psql \
    -X \
    -h "${DB_HOST}" \
    -p "${DB_PORT}" \
    -U "${DB_USER}" \
    -d "${DB_NAME}" \
    -v sign_up_user_id="${sign_up_user_id}" \
    -v first_problem_id="${first_problem_id}" \
    -v second_problem_id="${second_problem_id}" \
    -v third_problem_id="${third_problem_id}" \
    -v ON_ERROR_STOP=1 <<'SQL' >>"${test_log_temp_file}" 2>&1
INSERT INTO submissions(
    user_id,
    problem_id,
    problem_version,
    language,
    source_code,
    status,
    created_at,
    updated_at
)
VALUES
    (:'sign_up_user_id', :'first_problem_id', (SELECT version FROM problems WHERE problem_id = :'first_problem_id'), 'cpp17', 'accepted code', 'accepted', '2026-01-01T00:00:01Z', '2026-01-01T00:00:01Z'),
    (:'sign_up_user_id', :'second_problem_id', (SELECT version FROM problems WHERE problem_id = :'second_problem_id'), 'cpp17', 'wrong code', 'wrong_answer', '2026-01-01T00:00:02Z', '2026-01-01T00:00:02Z'),
    (:'sign_up_user_id', :'third_problem_id', (SELECT version FROM problems WHERE problem_id = :'third_problem_id'), 'cpp17', 'queued code', 'queued', '2026-01-01T00:00:03Z', '2026-01-01T00:00:03Z');

INSERT INTO user_problem_attempt_summary(
    user_id,
    problem_id,
    submission_count,
    accepted_submission_count,
    failed_submission_count,
    updated_at
)
VALUES
    (:'sign_up_user_id', :'first_problem_id', 1, 1, 0, NOW()),
    (:'sign_up_user_id', :'second_problem_id', 1, 0, 1, NOW()),
    (:'sign_up_user_id', :'third_problem_id', 1, 0, 0, NOW());

UPDATE problem_statistics
SET submission_count = 1, accepted_count = 1
WHERE problem_id = :'first_problem_id';

UPDATE problem_statistics
SET submission_count = 1, accepted_count = 0
WHERE problem_id = :'second_problem_id';

UPDATE problem_statistics
SET submission_count = 1, accepted_count = 0
WHERE problem_id = :'third_problem_id';
SQL
then
    append_log_line "${test_log_temp_file}" "problem list submission fixture insert failed"
    publish_failure_logs
    exit 1
fi

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem" \
    "${list_problem_response_file}" \
    "200" \
    "problem list"

if ! python3 - "${list_problem_response_file}" "${first_problem_id}" "${second_problem_id}" "${third_problem_id}" "${fourth_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
first_problem_id = int(sys.argv[2])
second_problem_id = int(sys.argv[3])
third_problem_id = int(sys.argv[4])
fourth_problem_id = int(sys.argv[5])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_count") != 4:
    raise SystemExit("problem_count mismatch")
if response.get("total_problem_count") != 4:
    raise SystemExit("total_problem_count mismatch")

expected_problems = [
    {
        "problem_id": fourth_problem_id,
        "title": "Queue Later",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 0,
        "accepted_count": 0,
        "user_problem_state": None,
    },
    {
        "problem_id": third_problem_id,
        "title": "Multiply",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 0,
        "user_problem_state": None,
    },
    {
        "problem_id": second_problem_id,
        "title": "plus one",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 0,
        "user_problem_state": None,
    },
    {
        "problem_id": first_problem_id,
        "title": "A Plus B",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 1,
        "user_problem_state": None,
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

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem" \
    "${authenticated_list_problem_response_file}" \
    "200" \
    "authenticated problem list" \
    "${sign_up_token}"

if ! python3 - "${authenticated_list_problem_response_file}" "${first_problem_id}" "${second_problem_id}" "${third_problem_id}" "${fourth_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
first_problem_id = int(sys.argv[2])
second_problem_id = int(sys.argv[3])
third_problem_id = int(sys.argv[4])
fourth_problem_id = int(sys.argv[5])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

expected_problems = [
    {
        "problem_id": fourth_problem_id,
        "title": "Queue Later",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 0,
        "accepted_count": 0,
        "user_problem_state": None,
    },
    {
        "problem_id": third_problem_id,
        "title": "Multiply",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 0,
        "user_problem_state": None,
    },
    {
        "problem_id": second_problem_id,
        "title": "plus one",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 0,
        "user_problem_state": "wrong",
    },
    {
        "problem_id": first_problem_id,
        "title": "A Plus B",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 1,
        "user_problem_state": "solved",
    },
]

if response.get("problem_count") != 4:
    raise SystemExit("authenticated problem_count mismatch")
if response.get("total_problem_count") != 4:
    raise SystemExit("authenticated total_problem_count mismatch")
if response.get("problems") != expected_problems:
    raise SystemExit("authenticated problem list mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "authenticated problem list response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "authenticated problem list response validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem?title=PLUS" \
    "${filtered_problem_response_file}" \
    "200" \
    "filtered problem list"

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
if response.get("total_problem_count") != 2:
    raise SystemExit("filtered total_problem_count mismatch")

expected_problems = [
    {
        "problem_id": second_problem_id,
        "title": "plus one",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 0,
        "user_problem_state": None,
    },
    {
        "problem_id": first_problem_id,
        "title": "A Plus B",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 1,
        "user_problem_state": None,
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

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem?problem_id=${second_problem_id}" \
    "${filtered_problem_id_response_file}" \
    "200" \
    "problem_id filtered problem list"

if ! python3 - "${filtered_problem_id_response_file}" "${second_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
second_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_count") != 1:
    raise SystemExit("problem_id filtered problem_count mismatch")
if response.get("total_problem_count") != 1:
    raise SystemExit("problem_id filtered total_problem_count mismatch")

expected_problems = [
    {
        "problem_id": second_problem_id,
        "title": "plus one",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 0,
        "user_problem_state": None,
    },
]
if response.get("problems") != expected_problems:
    raise SystemExit("problem_id filtered problem list mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "problem_id filtered problem list response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "problem list problem_id filter validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem?state=solved" \
    "${solved_problem_response_file}" \
    "200" \
    "solved problem list" \
    "${sign_up_token}"

if ! python3 - "${solved_problem_response_file}" "${first_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
first_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_count") != 1:
    raise SystemExit("solved problem_count mismatch")
if response.get("total_problem_count") != 1:
    raise SystemExit("solved total_problem_count mismatch")

expected_problems = [
    {
        "problem_id": first_problem_id,
        "title": "A Plus B",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 1,
        "user_problem_state": "solved",
    },
]
if response.get("problems") != expected_problems:
    raise SystemExit("solved problem list mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "solved problem list response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "problem list state filter validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem?sort=accepted_count&limit=2" \
    "${sorted_limited_problem_response_file}" \
    "200" \
    "sorted limited problem list"

if ! python3 - "${sorted_limited_problem_response_file}" "${first_problem_id}" "${second_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
first_problem_id = int(sys.argv[2])
second_problem_id = int(sys.argv[3])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_count") != 2:
    raise SystemExit("sorted limited problem_count mismatch")
if response.get("total_problem_count") != 4:
    raise SystemExit("sorted limited total_problem_count mismatch")

expected_problems = [
    {
        "problem_id": first_problem_id,
        "title": "A Plus B",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 1,
        "user_problem_state": None,
    },
    {
        "problem_id": second_problem_id,
        "title": "plus one",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 0,
        "user_problem_state": None,
    },
]
if response.get("problems") != expected_problems:
    raise SystemExit("sorted limited problem list mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "sorted limited problem list response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "problem list sort and limit validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem?sort=problem_id&direction=asc&limit=2&offset=1" \
    "${offset_problem_response_file}" \
    "200" \
    "offset problem list"

if ! python3 - "${offset_problem_response_file}" "${second_problem_id}" "${third_problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
second_problem_id = int(sys.argv[2])
third_problem_id = int(sys.argv[3])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_count") != 2:
    raise SystemExit("offset problem_count mismatch")
if response.get("total_problem_count") != 4:
    raise SystemExit("offset total_problem_count mismatch")

expected_problems = [
    {
        "problem_id": second_problem_id,
        "title": "plus one",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 0,
        "user_problem_state": None,
    },
    {
        "problem_id": third_problem_id,
        "title": "Multiply",
        "version": 1,
        "time_limit_ms": 0,
        "memory_limit_mb": 0,
        "submission_count": 1,
        "accepted_count": 0,
        "user_problem_state": None,
    },
]
if response.get("problems") != expected_problems:
    raise SystemExit("offset problem list mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "offset problem list response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "problem list offset validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem?state=solved" \
    "${invalid_state_problem_response_file}" \
    "400" \
    "invalid anonymous state problem list"
assert_json_error_code \
    "${invalid_state_problem_response_file}" \
    "invalid_query_parameter" \
    "invalid anonymous state problem list"
assert_json_error_message \
    "${invalid_state_problem_response_file}" \
    "invalid query parameter: state" \
    "invalid anonymous state problem list"
assert_json_error_field \
    "${invalid_state_problem_response_file}" \
    "state" \
    "invalid anonymous state problem list"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem?title=divide" \
    "${missing_problem_response_file}" \
    "200" \
    "missing title problem list"

if ! python3 - "${missing_problem_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_count") != 0:
    raise SystemExit("missing title problem_count mismatch")
if response.get("total_problem_count") != 0:
    raise SystemExit("missing title total_problem_count mismatch")
if response.get("problems") != []:
    raise SystemExit("missing title problems mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "missing title problem list response validation failed"
    publish_failure_logs
    exit 1
fi

print_success_log "problem list empty filter validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem?unsupported=value" \
    "${invalid_query_response_file}" \
    "400" \
    "invalid query problem list"
assert_json_error_code \
    "${invalid_query_response_file}" \
    "unsupported_query_parameter" \
    "invalid query problem list"
assert_json_error_message \
    "${invalid_query_response_file}" \
    "unsupported query parameter: unsupported" \
    "invalid query problem list"
assert_json_error_field \
    "${invalid_query_response_file}" \
    "unsupported" \
    "invalid query problem list"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem" \
    "${invalid_token_response_file}" \
    "401" \
    "invalid bearer token problem list" \
    "invalid-token"
assert_json_error_code \
    "${invalid_token_response_file}" \
    "invalid_or_expired_token" \
    "invalid bearer token problem list"
assert_json_error_message \
    "${invalid_token_response_file}" \
    "invalid, expired, or revoked token" \
    "invalid bearer token problem list"

append_log_line "${test_log_temp_file}" "problem list flow test passed"
print_success_log \
    "problem list flow test passed: first_problem_id=${first_problem_id}, second_problem_id=${second_problem_id}, third_problem_id=${third_problem_id}, fourth_problem_id=${fourth_problem_id}, user_id=${sign_up_user_id}"
