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

http_port="${SUBMISSION_FLOW_TEST_HTTP_PORT:-18084}"
base_url="${SUBMISSION_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${SUBMISSION_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${SUBMISSION_FLOW_TEST_LOGIN_ID:-$(make_test_login_id sf)}"
other_user_login_id="${SUBMISSION_FLOW_TEST_OTHER_LOGIN_ID:-$(make_test_login_id sf o)}"
admin_user_login_id="${SUBMISSION_FLOW_TEST_ADMIN_LOGIN_ID:-$(make_test_login_id sf a)}"
raw_password="${SUBMISSION_FLOW_TEST_PASSWORD:-password123}"
test_db_name="submission_flow_test_$$_$(date +%s)"
test_database_created="0"
submission_language="${SUBMISSION_FLOW_TEST_LANGUAGE:-cpp}"
second_submission_language="${SUBMISSION_FLOW_TEST_SECOND_LANGUAGE:-python}"
submission_source_code="${SUBMISSION_FLOW_TEST_SOURCE_CODE:-#include <iostream>
int main(){
    std::cout << 0 << '\n';
    return 0;
}
}"
second_submission_source_code="${SUBMISSION_FLOW_TEST_SECOND_SOURCE_CODE:-print(1)
}"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_submission_flow.log"
server_log_name="test_submission_flow_server.log"

init_flow_test
register_temp_file all_submission_response_file
register_temp_file submission_detail_response_file
register_temp_file missing_submission_response_file
register_temp_file list_submission_response_file
register_temp_file authenticated_list_submission_response_file
register_temp_file solved_state_submission_response_file
register_temp_file limited_submission_response_file
register_temp_file paged_submission_response_file
register_temp_file language_filtered_submission_response_file
register_temp_file invalid_limit_submission_response_file
register_temp_file invalid_top_submission_response_file
register_temp_file submission_history_response_file
register_temp_file missing_history_response_file
register_temp_file submission_source_response_file
register_temp_file unauthorized_source_response_file
register_temp_file forbidden_source_response_file
register_temp_file admin_source_response_file
register_temp_file missing_source_response_file
register_temp_file rejudge_response_file
register_temp_file rejudge_unauthorized_response_file
register_temp_file rejudge_invalid_response_file
register_temp_file rejudge_detail_response_file
register_temp_file problem_rejudge_response_file
register_temp_file problem_rejudge_unauthorized_response_file
register_temp_file problem_rejudge_missing_response_file
register_temp_file submission_ban_response_file
register_temp_file banned_submission_response_file
register_temp_file clear_submission_ban_response_file
register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file sign_up_response_file
register_temp_file submission_response_file
register_temp_file invalid_language_response_file

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
append_log_line "${test_log_temp_file}" "user_login_id=${user_login_id}"
append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"

apply_test_database_migrations
ensure_dedicated_http_server

read -r sign_up_user_id sign_up_token < <(
    sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "submission flow"
)
print_success_log "sign-up success"

read -r other_user_id other_user_token < <(
    sign_up_user "${other_user_login_id}" "${raw_password}" "${sign_up_response_file}" "submission flow"
)
print_success_log "other user sign-up success"

read -r admin_user_id admin_user_token < <(
    sign_up_user "${admin_user_login_id}" "${raw_password}" "${sign_up_response_file}" "submission flow"
)
print_success_log "admin user sign-up success"

promote_admin_user "${admin_user_id}" "submission flow" >/dev/null
print_success_log "admin user promote success"

problem_id="$(create_problem_in_db "submission flow")"
print_success_log "problem create success"

invalid_language_request_body="$(
    python3 - "${submission_source_code}" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "language": "ruby",
            "source_code": sys.argv[1],
        }
    )
)
PY
)"

invalid_language_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${invalid_language_response_file}" \
        "${sign_up_token}" \
        "${invalid_language_request_body}"
)"
assert_status_code \
    "${invalid_language_status_code}" \
    "400" \
    "${invalid_language_response_file}" \
    "invalid submission language"
assert_json_error_code \
    "${invalid_language_response_file}" \
    "invalid_field" \
    "invalid submission language"
assert_json_error_message \
    "${invalid_language_response_file}" \
    "unsupported language: ruby" \
    "invalid submission language"
assert_json_error_field \
    "${invalid_language_response_file}" \
    "language" \
    "invalid submission language"
print_success_log "invalid submission language validation success"

submission_request_body="$(
    python3 - "${submission_language}" "${submission_source_code}" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "language": sys.argv[1],
            "source_code": sys.argv[2],
        }
    )
)
PY
)"

submission_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${submission_response_file}" \
        "${sign_up_token}" \
        "${submission_request_body}"
)"
assert_status_code "${submission_status_code}" "201" "${submission_response_file}" "submission create"
print_success_log "submission create success"

submission_id="$(
    python3 - "${submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_response = json.load(response_file)

submission_id = submission_response.get("submission_id")
status = submission_response.get("status")

if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid submission_id in submission response")

if status != "queued":
    raise SystemExit("expected submission status to be queued")

print(submission_id)
PY
)"

second_submission_request_body="$(
    python3 - "${second_submission_language}" "${second_submission_source_code}" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "language": sys.argv[1],
            "source_code": sys.argv[2],
        }
    )
)
PY
)"

submission_ban_request_body="$(
    python3 <<'PY'
import json

print(json.dumps({"duration_minutes": 60}))
PY
)"

submission_ban_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/user/${sign_up_user_id}/submission-ban" \
        "${submission_ban_response_file}" \
        "${admin_user_token}" \
        "${submission_ban_request_body}"
)"
assert_status_code \
    "${submission_ban_status_code}" \
    "201" \
    "${submission_ban_response_file}" \
    "submission ban create"
python3 - "${submission_ban_response_file}" "${sign_up_user_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

user_id = response.get("user_id")
duration_minutes = response.get("duration_minutes")
submission_banned_until = response.get("submission_banned_until")

if user_id != int(sys.argv[2]):
    raise SystemExit("unexpected user_id in submission ban response")
if duration_minutes != 60:
    raise SystemExit("unexpected duration_minutes in submission ban response")
if not isinstance(submission_banned_until, str) or not submission_banned_until:
    raise SystemExit("missing submission_banned_until in submission ban response")
PY
print_success_log "submission ban create success"

banned_submission_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${banned_submission_response_file}" \
        "${sign_up_token}" \
        "${second_submission_request_body}"
)"
assert_status_code \
    "${banned_submission_status_code}" \
    "403" \
    "${banned_submission_response_file}" \
    "banned submission create"
assert_json_error_code \
    "${banned_submission_response_file}" \
    "submission_banned" \
    "banned submission create"
assert_json_error_message \
    "${banned_submission_response_file}" \
    "submission is currently banned" \
    "banned submission create"
print_success_log "banned submission blocked success"

clear_submission_ban_status_code="$(
    send_http_request \
        "DELETE" \
        "${base_url}/api/user/${sign_up_user_id}/submission-ban" \
        "${clear_submission_ban_response_file}" \
        "${admin_user_token}"
)"
assert_status_code \
    "${clear_submission_ban_status_code}" \
    "200" \
    "${clear_submission_ban_response_file}" \
    "submission ban clear"
print_success_log "submission ban clear success"

second_submission_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${submission_response_file}" \
        "${sign_up_token}" \
        "${second_submission_request_body}"
)"
assert_status_code "${second_submission_status_code}" "201" "${submission_response_file}" "second submission create"
print_success_log "second submission create success"

second_submission_id="$(
    python3 - "${submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_response = json.load(response_file)

submission_id = submission_response.get("submission_id")
status = submission_response.get("status")

if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid submission_id in second submission response")

if status != "queued":
    raise SystemExit("expected second submission status to be queued")

print(submission_id)
PY
)"

missing_submission_id=$((second_submission_id + 999999))

mapfile -t initial_queue_priority_values < <(
    PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v submission_id="${submission_id}" \
        -v second_submission_id="${second_submission_id}" \
        -v ON_ERROR_STOP=1 \
        -qAt <<'SQL'
SELECT priority
FROM submission_queue
WHERE submission_id = :'submission_id';

SELECT priority
FROM submission_queue
WHERE submission_id = :'second_submission_id';
SQL
)

if [[ "${#initial_queue_priority_values[@]}" -ne 2 ]]; then
    append_log_line "${test_log_temp_file}" "initial queue priority validation failed: unexpected row count"
    publish_failure_logs
    echo "initial queue priority validation failed: expected 2 rows, got ${#initial_queue_priority_values[@]}" >&2
    exit 1
fi

if [[ "${initial_queue_priority_values[0]}" != "100" ]]; then
    append_log_line "${test_log_temp_file}" "initial queue priority validation failed: first_priority=${initial_queue_priority_values[0]}"
    publish_failure_logs
    echo "initial queue priority validation failed: expected first priority 100, got ${initial_queue_priority_values[0]}" >&2
    exit 1
fi

if [[ "${initial_queue_priority_values[1]}" != "100" ]]; then
    append_log_line "${test_log_temp_file}" "initial queue priority validation failed: second_priority=${initial_queue_priority_values[1]}"
    publish_failure_logs
    echo "initial queue priority validation failed: expected second priority 100, got ${initial_queue_priority_values[1]}" >&2
    exit 1
fi

print_success_log "initial queue priority validation success"

submission_detail_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${submission_id}" \
        "${submission_detail_response_file}"
)"
assert_status_code "${submission_detail_status_code}" "200" "${submission_detail_response_file}" "submission detail get"
print_success_log "submission detail get success"

missing_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${missing_submission_id}" \
        "${missing_submission_response_file}"
)"
assert_status_code "${missing_submission_status_code}" "404" "${missing_submission_response_file}" "missing submission get"
print_success_log "missing submission get success"

submission_history_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${submission_id}/history" \
        "${submission_history_response_file}" \
        "${admin_user_token}"
)"
assert_status_code "${submission_history_status_code}" "200" "${submission_history_response_file}" "submission history get"
print_success_log "submission history get success"

missing_history_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${missing_submission_id}/history" \
        "${missing_history_response_file}" \
        "${admin_user_token}"
)"
assert_status_code "${missing_history_status_code}" "404" "${missing_history_response_file}" "missing submission history get"
print_success_log "missing submission history get success"

submission_source_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${submission_id}/source" \
        "${submission_source_response_file}" \
        "${sign_up_token}"
)"
assert_status_code "${submission_source_status_code}" "200" "${submission_source_response_file}" "submission source get"
print_success_log "submission source get success"

unauthorized_source_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${submission_id}/source" \
        "${unauthorized_source_response_file}"
)"
assert_status_code "${unauthorized_source_status_code}" "401" "${unauthorized_source_response_file}" "unauthorized submission source get"
print_success_log "unauthorized submission source get success"

forbidden_source_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${submission_id}/source" \
        "${forbidden_source_response_file}" \
        "${other_user_token}"
)"
assert_status_code "${forbidden_source_status_code}" "403" "${forbidden_source_response_file}" "forbidden submission source get"
print_success_log "forbidden submission source get success"

admin_source_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${submission_id}/source" \
        "${admin_source_response_file}" \
        "${admin_user_token}"
)"
assert_status_code "${admin_source_status_code}" "200" "${admin_source_response_file}" "admin submission source get"
print_success_log "admin submission source get success"

missing_source_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${missing_submission_id}/source" \
        "${missing_source_response_file}" \
        "${sign_up_token}"
)"
assert_status_code "${missing_source_status_code}" "404" "${missing_source_response_file}" "missing submission source get"
print_success_log "missing submission source get success"

if ! PGPASSWORD="${DB_PASSWORD}" psql \
    -X \
    -h "${DB_HOST}" \
    -p "${DB_PORT}" \
    -U "${DB_USER}" \
    -d "${DB_NAME}" \
    -v submission_id="${submission_id}" \
    -v problem_id="${problem_id}" \
    -v ON_ERROR_STOP=1 >/dev/null <<'SQL'
UPDATE submissions
SET
    status = 'accepted',
    score = 100,
    compile_output = 'stale compile output',
    judge_output = 'stale judge output',
    elapsed_ms = 123,
    max_rss_kb = 456,
    updated_at = NOW()
WHERE submission_id = :'submission_id';

DELETE FROM submission_queue
WHERE submission_id = :'submission_id';

UPDATE problem_statistics
SET accepted_count = accepted_count + 1, updated_at = NOW()
WHERE problem_id = :'problem_id';

INSERT INTO submission_status_history(submission_id, from_status, to_status, reason)
VALUES(
    :'submission_id',
    'queued',
    'accepted',
    'submission flow rejudge setup'
);
SQL
then
    append_log_line "${test_log_temp_file}" "rejudge setup failed"
    publish_failure_logs
    echo "submission rejudge setup failed" >&2
    exit 1
fi

rejudge_unauthorized_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${submission_id}/rejudge" \
        "${rejudge_unauthorized_response_file}" \
        "${sign_up_token}"
)"
assert_status_code "${rejudge_unauthorized_status_code}" "401" "${rejudge_unauthorized_response_file}" "unauthorized rejudge"
print_success_log "unauthorized rejudge success"

rejudge_invalid_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${second_submission_id}/rejudge" \
        "${rejudge_invalid_response_file}" \
        "${admin_user_token}"
)"
assert_status_code "${rejudge_invalid_status_code}" "400" "${rejudge_invalid_response_file}" "queued submission rejudge"
print_success_log "queued submission rejudge success"

rejudge_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${submission_id}/rejudge" \
        "${rejudge_response_file}" \
        "${admin_user_token}"
)"
assert_status_code "${rejudge_status_code}" "200" "${rejudge_response_file}" "submission rejudge"
print_success_log "submission rejudge success"

rejudge_detail_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${submission_id}" \
        "${rejudge_detail_response_file}"
)"
assert_status_code "${rejudge_detail_status_code}" "200" "${rejudge_detail_response_file}" "rejudge detail get"
print_success_log "rejudge detail get success"

if ! python3 \
    - "${rejudge_unauthorized_response_file}" \
    "${rejudge_invalid_response_file}" \
    "${rejudge_response_file}" \
    "${rejudge_detail_response_file}" \
    "${submission_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    unauthorized_response = json.load(response_file)

with open(sys.argv[2], encoding="utf-8") as response_file:
    invalid_response = json.load(response_file)

with open(sys.argv[3], encoding="utf-8") as response_file:
    rejudge_response = json.load(response_file)

with open(sys.argv[4], encoding="utf-8") as response_file:
    detail_response = json.load(response_file)

expected_submission_id = int(sys.argv[5])

if unauthorized_response.get("error", {}).get("code") != "admin_bearer_token_required":
    raise SystemExit("unexpected error code for unauthorized rejudge response")

if invalid_response.get("error", {}).get("code") != "bad_request":
    raise SystemExit("unexpected error code for queued rejudge response")

if rejudge_response.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in rejudge response")

if rejudge_response.get("status") != "queued":
    raise SystemExit("expected rejudge response status to be queued")

if detail_response.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in rejudge detail response")

if detail_response.get("status") != "queued":
    raise SystemExit("expected rejudge detail status to be queued")

if detail_response.get("score", "missing") is not None:
    raise SystemExit("expected rejudge detail score to be null")

if detail_response.get("compile_output", "missing") is not None:
    raise SystemExit("expected rejudge detail compile_output to be null")

if detail_response.get("judge_output", "missing") is not None:
    raise SystemExit("expected rejudge detail judge_output to be null")

if detail_response.get("elapsed_ms", "missing") is not None:
    raise SystemExit("expected rejudge detail elapsed_ms to be null")

if detail_response.get("max_rss_kb", "missing") is not None:
    raise SystemExit("expected rejudge detail max_rss_kb to be null")
PY
then
    append_log_line "${test_log_temp_file}" "submission rejudge validation failed"
    publish_failure_logs
    exit 1
fi

mapfile -t rejudge_db_values < <(
    PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v submission_id="${submission_id}" \
        -v problem_id="${problem_id}" \
        -v ON_ERROR_STOP=1 \
        -qAt <<'SQL'
SELECT accepted_count
FROM problem_statistics
WHERE problem_id = :'problem_id';

SELECT COUNT(*)
FROM submission_queue
WHERE submission_id = :'submission_id';

SELECT priority
FROM submission_queue
WHERE submission_id = :'submission_id';
SQL
)

if [[ "${#rejudge_db_values[@]}" -ne 3 ]]; then
    append_log_line "${test_log_temp_file}" "rejudge db validation failed: unexpected row count"
    publish_failure_logs
    echo "rejudge db validation failed: expected 3 rows, got ${#rejudge_db_values[@]}" >&2
    exit 1
fi

if [[ "${rejudge_db_values[0]}" != "0" ]]; then
    append_log_line "${test_log_temp_file}" "rejudge db validation failed: accepted_count=${rejudge_db_values[0]}"
    publish_failure_logs
    echo "rejudge db validation failed: expected accepted_count 0, got ${rejudge_db_values[0]}" >&2
    exit 1
fi

if [[ "${rejudge_db_values[1]}" != "1" ]]; then
    append_log_line "${test_log_temp_file}" "rejudge db validation failed: queue_count=${rejudge_db_values[1]}"
    publish_failure_logs
    echo "rejudge db validation failed: expected queue row count 1, got ${rejudge_db_values[1]}" >&2
    exit 1
fi

if [[ "${rejudge_db_values[2]}" != "0" ]]; then
    append_log_line "${test_log_temp_file}" "rejudge db validation failed: queue_priority=${rejudge_db_values[2]}"
    publish_failure_logs
    echo "rejudge db validation failed: expected queue priority 0, got ${rejudge_db_values[2]}" >&2
    exit 1
fi

print_success_log "submission rejudge db validation success"

all_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission" \
        "${all_submission_response_file}"
)"
assert_status_code "${all_submission_status_code}" "200" "${all_submission_response_file}" "submission list without filter"
print_success_log "submission list without filter success"

list_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission?user_id=${sign_up_user_id}&problem_id=${problem_id}&status=queued" \
        "${list_submission_response_file}"
)"
assert_status_code "${list_submission_status_code}" "200" "${list_submission_response_file}" "submission list"
print_success_log "submission list success"

authenticated_list_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission?user_id=${sign_up_user_id}&problem_id=${problem_id}&status=queued" \
        "${authenticated_list_submission_response_file}" \
        "${sign_up_token}"
)"
assert_status_code \
    "${authenticated_list_submission_status_code}" \
    "200" \
    "${authenticated_list_submission_response_file}" \
    "authenticated submission list"
print_success_log "authenticated submission list success"

language_filtered_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission?user_id=${sign_up_user_id}&problem_id=${problem_id}&language=${second_submission_language}" \
        "${language_filtered_submission_response_file}"
)"
assert_status_code \
    "${language_filtered_submission_status_code}" \
    "200" \
    "${language_filtered_submission_response_file}" \
    "language filtered submission list"
print_success_log "language filtered submission list success"

limited_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission?user_id=${sign_up_user_id}&problem_id=${problem_id}&limit=1" \
        "${limited_submission_response_file}"
)"
assert_status_code "${limited_submission_status_code}" "200" "${limited_submission_response_file}" "submission limited list"
print_success_log "submission limited list success"

paged_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission?user_id=${sign_up_user_id}&problem_id=${problem_id}&limit=1&page=2" \
        "${paged_submission_response_file}"
)"
assert_status_code "${paged_submission_status_code}" "200" "${paged_submission_response_file}" "submission paged list"
print_success_log "submission paged list success"

invalid_limit_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission?limit=0" \
        "${invalid_limit_submission_response_file}"
)"
assert_status_code "${invalid_limit_submission_status_code}" "400" "${invalid_limit_submission_response_file}" "submission invalid limit list"
print_success_log "submission invalid limit list success"

invalid_top_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission?top=${submission_id}" \
        "${invalid_top_submission_response_file}"
)"
assert_status_code "${invalid_top_submission_status_code}" "400" "${invalid_top_submission_response_file}" "submission invalid top list"
print_success_log "submission invalid top list success"

if ! python3 \
    - "${submission_history_response_file}" \
    "${submission_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_history = json.load(response_file)

expected_submission_id = int(sys.argv[2])

if submission_history.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in submission history response")

if submission_history.get("history_count") != 1:
    raise SystemExit("expected history_count to be 1 in submission history response")

histories = submission_history.get("histories")
if not isinstance(histories, list) or len(histories) != 1:
    raise SystemExit("expected exactly one history row in submission history response")

history = histories[0]

history_id = history.get("history_id")
if not isinstance(history_id, int) or history_id <= 0:
    raise SystemExit("invalid history_id in submission history response")

if history.get("from_status", "missing") is not None:
    raise SystemExit("expected from_status to be null in submission history response")

if history.get("to_status") != "queued":
    raise SystemExit("expected to_status to be queued in submission history response")

if history.get("reason", "missing") is not None:
    raise SystemExit("expected reason to be null in submission history response")

created_at = history.get("created_at")
if not isinstance(created_at, str) or not created_at:
    raise SystemExit("missing created_at in submission history response")
PY
then
    append_log_line "${test_log_temp_file}" "submission history validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${missing_history_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("error", {}).get("code") != "not_found":
    raise SystemExit("unexpected error code for missing submission history response")
PY
then
    append_log_line "${test_log_temp_file}" "submission history error validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${submission_source_response_file}" \
    "${submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${submission_language}" \
    "${submission_source_code}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_source = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_user_id = int(sys.argv[3])
expected_problem_id = int(sys.argv[4])
expected_language = sys.argv[5]
expected_source_code = sys.argv[6]

if submission_source.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in submission source response")

if submission_source.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch in submission source response")

if submission_source.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch in submission source response")

if submission_source.get("language") != expected_language:
    raise SystemExit("language mismatch in submission source response")

if submission_source.get("source_code") != expected_source_code:
    raise SystemExit("source_code mismatch in submission source response")

if submission_source.get("compile_output", "missing") is not None:
    raise SystemExit("expected compile_output to be null in submission source response")

if submission_source.get("judge_output", "missing") is not None:
    raise SystemExit("expected judge_output to be null in submission source response")
PY
then
    append_log_line "${test_log_temp_file}" "submission source validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${unauthorized_source_response_file}" \
    "${forbidden_source_response_file}" \
    "${missing_source_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    unauthorized_response = json.load(response_file)

with open(sys.argv[2], encoding="utf-8") as response_file:
    forbidden_response = json.load(response_file)

with open(sys.argv[3], encoding="utf-8") as response_file:
    missing_response = json.load(response_file)

if unauthorized_response.get("error", {}).get("code") != "missing_or_invalid_bearer_token":
    raise SystemExit("unexpected error code for unauthorized source response")

if forbidden_response.get("error", {}).get("code") != "forbidden":
    raise SystemExit("unexpected error code for forbidden source response")

if missing_response.get("error", {}).get("code") != "not_found":
    raise SystemExit("unexpected error code for missing source response")
PY
then
    append_log_line "${test_log_temp_file}" "submission source error validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${admin_source_response_file}" \
    "${submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${submission_language}" \
    "${submission_source_code}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_source = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_user_id = int(sys.argv[3])
expected_problem_id = int(sys.argv[4])
expected_language = sys.argv[5]
expected_source_code = sys.argv[6]

if submission_source.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in admin submission source response")

if submission_source.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch in admin submission source response")

if submission_source.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch in admin submission source response")

if submission_source.get("language") != expected_language:
    raise SystemExit("language mismatch in admin submission source response")

if submission_source.get("source_code") != expected_source_code:
    raise SystemExit("source_code mismatch in admin submission source response")

if submission_source.get("compile_output", "missing") is not None:
    raise SystemExit("expected compile_output to be null in admin submission source response")

if submission_source.get("judge_output", "missing") is not None:
    raise SystemExit("expected judge_output to be null in admin submission source response")
PY
then
    append_log_line "${test_log_temp_file}" "admin submission source validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${all_submission_response_file}" \
    "${submission_id}" \
    "${second_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${submission_language}" \
    "${second_submission_language}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_list = json.load(response_file)

expected_first_submission_id = int(sys.argv[2])
expected_second_submission_id = int(sys.argv[3])
expected_user_id = int(sys.argv[4])
expected_problem_id = int(sys.argv[5])
expected_older_language = sys.argv[6]
expected_latest_language = sys.argv[7]

submission_count = submission_list.get("submission_count")
if not isinstance(submission_count, int) or submission_count < 2:
    raise SystemExit("expected submission_count to be at least 2")

if submission_list.get("total_submission_count") != 2:
    raise SystemExit("expected total_submission_count to be 2")

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) < 2:
    raise SystemExit("expected at least two submissions in unfiltered submission list response")

latest_submission = submissions[0]
older_submission = submissions[1]

if latest_submission.get("submission_id") != expected_second_submission_id:
    raise SystemExit("expected newest submission to appear first in unfiltered list")

if older_submission.get("submission_id") != expected_first_submission_id:
    raise SystemExit("expected second newest submission to appear second in unfiltered list")

expected_languages_by_submission_id = {
    expected_first_submission_id: expected_older_language,
    expected_second_submission_id: expected_latest_language,
}

for submission in (latest_submission, older_submission):
    if submission.get("user_id") != expected_user_id:
        raise SystemExit("user_id mismatch in unfiltered submission list response")

    if submission.get("problem_id") != expected_problem_id:
        raise SystemExit("problem_id mismatch in unfiltered submission list response")

    expected_language = expected_languages_by_submission_id.get(submission.get("submission_id"))
    if submission.get("language") != expected_language:
        raise SystemExit("language mismatch in unfiltered submission list response")

    if submission.get("status") != "queued":
        raise SystemExit("expected unfiltered submission list status to be queued")

    if submission.get("score", "missing") is not None:
        raise SystemExit("expected unfiltered submission list score to be null before judging")

    if submission.get("elapsed_ms", "missing") is not None:
        raise SystemExit("expected unfiltered submission list elapsed_ms to be null before judging")

    if submission.get("max_rss_kb", "missing") is not None:
        raise SystemExit("expected unfiltered submission list max_rss_kb to be null before judging")

    if submission.get("user_problem_state", "missing") is not None:
        raise SystemExit("expected unfiltered submission list user_problem_state to be null")

    if "source_code" in submission:
        raise SystemExit("unfiltered submission list response must not expose source_code")
PY
then
    append_log_line "${test_log_temp_file}" "submission list without filter validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${submission_detail_response_file}" \
    "${submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${submission_language}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_detail = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_user_id = int(sys.argv[3])
expected_problem_id = int(sys.argv[4])
expected_language = sys.argv[5]

if submission_detail.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in submission detail response")

if submission_detail.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch in submission detail response")

if submission_detail.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch in submission detail response")

if submission_detail.get("language") != expected_language:
    raise SystemExit("language mismatch in submission detail response")

if submission_detail.get("status") != "queued":
    raise SystemExit("expected submission detail status to be queued")

if submission_detail.get("score", "missing") is not None:
    raise SystemExit("expected submission detail score to be null before judging")

if submission_detail.get("compile_output", "missing") is not None:
    raise SystemExit("expected compile_output to be null before judging")

if submission_detail.get("judge_output", "missing") is not None:
    raise SystemExit("expected judge_output to be null before judging")

if submission_detail.get("elapsed_ms", "missing") is not None:
    raise SystemExit("expected elapsed_ms to be null before judging")

if submission_detail.get("max_rss_kb", "missing") is not None:
    raise SystemExit("expected max_rss_kb to be null before judging")

created_at = submission_detail.get("created_at")
updated_at = submission_detail.get("updated_at")

if not isinstance(created_at, str) or not created_at:
    raise SystemExit("missing created_at in submission detail response")

if not isinstance(updated_at, str) or not updated_at:
    raise SystemExit("missing updated_at in submission detail response")

if "source_code" in submission_detail:
    raise SystemExit("submission detail response must not expose source_code")
PY
then
    append_log_line "${test_log_temp_file}" "submission detail validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${list_submission_response_file}" \
    "${submission_id}" \
    "${second_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${submission_language}" \
    "${second_submission_language}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_list = json.load(response_file)

expected_first_submission_id = int(sys.argv[2])
expected_second_submission_id = int(sys.argv[3])
expected_user_id = int(sys.argv[4])
expected_problem_id = int(sys.argv[5])
expected_older_language = sys.argv[6]
expected_latest_language = sys.argv[7]

if submission_list.get("submission_count") != 2:
    raise SystemExit("expected submission_count to be 2")

if submission_list.get("total_submission_count") != 2:
    raise SystemExit("expected total_submission_count to be 2")

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) != 2:
    raise SystemExit("expected two submissions in submission list response")

latest_submission = submissions[0]
older_submission = submissions[1]

if latest_submission.get("submission_id") != expected_second_submission_id:
    raise SystemExit("expected newest submission to appear first")

if older_submission.get("submission_id") != expected_first_submission_id:
    raise SystemExit("expected older submission to appear second")

expected_languages_by_submission_id = {
    expected_first_submission_id: expected_older_language,
    expected_second_submission_id: expected_latest_language,
}

for submission in submissions:
    if submission.get("user_id") != expected_user_id:
        raise SystemExit("user_id mismatch in submission list response")

    if submission.get("problem_id") != expected_problem_id:
        raise SystemExit("problem_id mismatch in submission list response")

    expected_language = expected_languages_by_submission_id.get(submission.get("submission_id"))
    if submission.get("language") != expected_language:
        raise SystemExit("language mismatch in submission list response")

    if submission.get("status") != "queued":
        raise SystemExit("expected submission list status to be queued")

    if submission.get("score", "missing") is not None:
        raise SystemExit("expected submission list score to be null before judging")

    if submission.get("elapsed_ms", "missing") is not None:
        raise SystemExit("expected submission list elapsed_ms to be null before judging")

    if submission.get("max_rss_kb", "missing") is not None:
        raise SystemExit("expected submission list max_rss_kb to be null before judging")

    created_at = submission.get("created_at")
    updated_at = submission.get("updated_at")

    if not isinstance(created_at, str) or not created_at:
        raise SystemExit("missing created_at in submission list response")

    if not isinstance(updated_at, str) or not updated_at:
        raise SystemExit("missing updated_at in submission list response")

    if "source_code" in submission:
        raise SystemExit("submission list response must not expose source_code")
PY
then
    append_log_line "${test_log_temp_file}" "submission list validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${authenticated_list_submission_response_file}" \
    "${submission_id}" \
    "${second_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${submission_language}" \
    "${second_submission_language}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_list = json.load(response_file)

expected_first_submission_id = int(sys.argv[2])
expected_second_submission_id = int(sys.argv[3])
expected_user_id = int(sys.argv[4])
expected_problem_id = int(sys.argv[5])
expected_older_language = sys.argv[6]
expected_latest_language = sys.argv[7]

if submission_list.get("submission_count") != 2:
    raise SystemExit("expected authenticated submission_count to be 2")

if submission_list.get("total_submission_count") != 2:
    raise SystemExit("expected authenticated total_submission_count to be 2")

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) != 2:
    raise SystemExit("expected two submissions in authenticated submission list response")

latest_submission = submissions[0]
older_submission = submissions[1]

if latest_submission.get("submission_id") != expected_second_submission_id:
    raise SystemExit("expected newest submission to appear first in authenticated list")

if older_submission.get("submission_id") != expected_first_submission_id:
    raise SystemExit("expected older submission to appear second in authenticated list")

expected_languages_by_submission_id = {
    expected_first_submission_id: expected_older_language,
    expected_second_submission_id: expected_latest_language,
}

for submission in submissions:
    if submission.get("user_id") != expected_user_id:
        raise SystemExit("user_id mismatch in authenticated submission list response")

    if submission.get("problem_id") != expected_problem_id:
        raise SystemExit("problem_id mismatch in authenticated submission list response")

    expected_language = expected_languages_by_submission_id.get(submission.get("submission_id"))
    if submission.get("language") != expected_language:
        raise SystemExit("language mismatch in authenticated submission list response")

    if submission.get("status") != "queued":
        raise SystemExit("expected authenticated submission list status to be queued")

    if submission.get("user_problem_state", "missing") is not None:
        raise SystemExit("expected authenticated queued submission list user_problem_state to be null")
PY
then
    append_log_line "${test_log_temp_file}" "authenticated submission list validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${language_filtered_submission_response_file}" \
    "${second_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${second_submission_language}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_list = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_user_id = int(sys.argv[3])
expected_problem_id = int(sys.argv[4])
expected_language = sys.argv[5]

if submission_list.get("submission_count") != 1:
    raise SystemExit("expected submission_count to be 1 for language filtered submission list response")

if submission_list.get("total_submission_count") != 1:
    raise SystemExit("expected total_submission_count to be 1 for language filtered submission list response")

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) != 1:
    raise SystemExit("expected one submission in language filtered submission list response")

submission = submissions[0]

if submission.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in language filtered submission list response")

if submission.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch in language filtered submission list response")

if submission.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch in language filtered submission list response")

if submission.get("language") != expected_language:
    raise SystemExit("language mismatch in language filtered submission list response")
PY
then
    append_log_line "${test_log_temp_file}" "language filtered submission list validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${limited_submission_response_file}" \
    "${second_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${second_submission_language}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_list = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_user_id = int(sys.argv[3])
expected_problem_id = int(sys.argv[4])
expected_language = sys.argv[5]

if submission_list.get("submission_count") != 1:
    raise SystemExit("expected submission_count to be 1 for limited submission list response")

if submission_list.get("total_submission_count") != 2:
    raise SystemExit("expected total_submission_count to be 2 for limited submission list response")

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) != 1:
    raise SystemExit("expected one submission in limited submission list response")

submission = submissions[0]

if submission.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in limited submission list response")

if submission.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch in limited submission list response")

if submission.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch in limited submission list response")

if submission.get("language") != expected_language:
    raise SystemExit("language mismatch in limited submission list response")
PY
then
    append_log_line "${test_log_temp_file}" "submission limited list validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${paged_submission_response_file}" \
    "${submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${submission_language}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_list = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_user_id = int(sys.argv[3])
expected_problem_id = int(sys.argv[4])
expected_language = sys.argv[5]

if submission_list.get("submission_count") != 1:
    raise SystemExit("expected submission_count to be 1 for paged submission list response")

if submission_list.get("total_submission_count") != 2:
    raise SystemExit("expected total_submission_count to be 2 for paged submission list response")

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) != 1:
    raise SystemExit("expected one submission in paged submission list response")

submission = submissions[0]

if submission.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in paged submission list response")

if submission.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch in paged submission list response")

if submission.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch in paged submission list response")

if submission.get("language") != expected_language:
    raise SystemExit("language mismatch in paged submission list response")
PY
then
    append_log_line "${test_log_temp_file}" "submission paged list validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${invalid_top_submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

error = response.get("error", {})

if error.get("code") != "unsupported_query_parameter":
    raise SystemExit("unexpected error code for invalid top submission list response")

if error.get("message") != "unsupported query parameter: top":
    raise SystemExit("unexpected error message for invalid top submission list response")

if error.get("field") != "top":
    raise SystemExit("unexpected error field for invalid top submission list response")
PY
then
    append_log_line "${test_log_temp_file}" "submission invalid top validation failed"
    publish_failure_logs
    exit 1
fi

if ! python3 \
    - "${invalid_limit_submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

error = response.get("error", {})

if error.get("code") != "invalid_query_parameter":
    raise SystemExit("unexpected error code for invalid limit submission list response")

if error.get("message") != "invalid query parameter: limit":
    raise SystemExit("unexpected error message for invalid limit submission list response")

if error.get("field") != "limit":
    raise SystemExit("unexpected error field for invalid limit submission list response")
PY
then
    append_log_line "${test_log_temp_file}" "submission invalid limit validation failed"
    publish_failure_logs
    exit 1
fi

third_submission_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${submission_response_file}" \
        "${sign_up_token}" \
        "${submission_request_body}"
)"
assert_status_code "${third_submission_status_code}" "201" "${submission_response_file}" "third submission create"

third_submission_id="$(
    python3 - "${submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_response = json.load(response_file)

submission_id = submission_response.get("submission_id")
if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid third submission_id")

print(submission_id)
PY
)"

if ! PGPASSWORD="${DB_PASSWORD}" psql \
    -X \
    -h "${DB_HOST}" \
    -p "${DB_PORT}" \
    -U "${DB_USER}" \
    -d "${DB_NAME}" \
    -v first_submission_id="${submission_id}" \
    -v second_submission_id="${second_submission_id}" \
    -v third_submission_id="${third_submission_id}" \
    -v problem_id="${problem_id}" \
    -v ON_ERROR_STOP=1 >/dev/null <<'SQL'
UPDATE submissions
SET
    status = 'accepted',
    score = 100,
    compile_output = 'accepted stale compile output',
    judge_output = 'accepted stale judge output',
    elapsed_ms = 11,
    max_rss_kb = 22,
    updated_at = NOW()
WHERE submission_id = :'first_submission_id';

UPDATE submissions
SET
    status = 'wrong_answer',
    score = 0,
    compile_output = 'wrong answer stale compile output',
    judge_output = 'wrong answer stale judge output',
    elapsed_ms = 33,
    max_rss_kb = 44,
    updated_at = NOW()
WHERE submission_id = :'second_submission_id';

UPDATE submissions
SET
    status = 'compile_error',
    score = 0,
    compile_output = 'compile error stale compile output',
    judge_output = 'compile error stale judge output',
    elapsed_ms = NULL,
    max_rss_kb = NULL,
    updated_at = NOW()
WHERE submission_id = :'third_submission_id';

DELETE FROM submission_queue
WHERE submission_id IN (
    :'first_submission_id',
    :'second_submission_id',
    :'third_submission_id'
);

UPDATE problem_statistics
SET accepted_count = accepted_count + 1, updated_at = NOW()
WHERE problem_id = :'problem_id';
SQL
then
    append_log_line "${test_log_temp_file}" "problem rejudge setup failed"
    publish_failure_logs
    echo "problem rejudge setup failed" >&2
    exit 1
fi

solved_state_submission_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission?user_id=${sign_up_user_id}&problem_id=${problem_id}" \
        "${solved_state_submission_response_file}" \
        "${sign_up_token}"
)"
assert_status_code \
    "${solved_state_submission_status_code}" \
    "200" \
    "${solved_state_submission_response_file}" \
    "solved state submission list"
print_success_log "solved state submission list success"

if ! python3 \
    - "${solved_state_submission_response_file}" \
    "${submission_id}" \
    "${second_submission_id}" \
    "${third_submission_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_list = json.load(response_file)

expected_submission_ids = {
    int(sys.argv[2]),
    int(sys.argv[3]),
    int(sys.argv[4]),
}

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) != 3:
    raise SystemExit("expected three submissions in solved state submission list response")

returned_submission_ids = {submission.get("submission_id") for submission in submissions}
if returned_submission_ids != expected_submission_ids:
    raise SystemExit("submission ids mismatch in solved state submission list response")

for submission in submissions:
    if submission.get("user_problem_state") != "solved":
        raise SystemExit("expected solved user_problem_state in solved state submission list response")
PY
then
    append_log_line "${test_log_temp_file}" "solved state submission list validation failed"
    publish_failure_logs
    exit 1
fi

problem_rejudge_unauthorized_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/problem/${problem_id}/rejudge" \
        "${problem_rejudge_unauthorized_response_file}" \
        "${sign_up_token}"
)"
assert_status_code \
    "${problem_rejudge_unauthorized_status_code}" \
    "401" \
    "${problem_rejudge_unauthorized_response_file}" \
    "problem rejudge unauthorized"

problem_rejudge_missing_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/problem/${missing_submission_id}/rejudge" \
        "${problem_rejudge_missing_response_file}" \
        "${admin_user_token}"
)"
assert_status_code \
    "${problem_rejudge_missing_status_code}" \
    "404" \
    "${problem_rejudge_missing_response_file}" \
    "problem rejudge missing"

problem_rejudge_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/problem/${problem_id}/rejudge" \
        "${problem_rejudge_response_file}" \
        "${admin_user_token}"
)"
assert_status_code \
    "${problem_rejudge_status_code}" \
    "200" \
    "${problem_rejudge_response_file}" \
    "problem rejudge"

if ! python3 \
    - "${problem_rejudge_unauthorized_response_file}" \
    "${problem_rejudge_missing_response_file}" \
    "${problem_rejudge_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    unauthorized_response = json.load(response_file)

with open(sys.argv[2], encoding="utf-8") as response_file:
    missing_response = json.load(response_file)

with open(sys.argv[3], encoding="utf-8") as response_file:
    success_response = json.load(response_file)

if unauthorized_response.get("error", {}).get("code") != "admin_bearer_token_required":
    raise SystemExit("unexpected error code for unauthorized problem rejudge response")

if missing_response.get("error", {}).get("code") != "problem_not_found":
    raise SystemExit("unexpected error code for missing problem rejudge response")

if success_response.get("message") != "problem submissions requeued":
    raise SystemExit("unexpected success message for problem rejudge response")
PY
then
    append_log_line "${test_log_temp_file}" "problem rejudge response validation failed"
    publish_failure_logs
    exit 1
fi

mapfile -t problem_rejudge_values < <(
    PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v first_submission_id="${submission_id}" \
        -v second_submission_id="${second_submission_id}" \
        -v third_submission_id="${third_submission_id}" \
        -v problem_id="${problem_id}" \
        -v ON_ERROR_STOP=1 \
        -qAt <<'SQL'
SELECT status::text, score, compile_output, judge_output, elapsed_ms, max_rss_kb
FROM submissions
WHERE submission_id = :'first_submission_id';

SELECT status::text, score, compile_output, judge_output, elapsed_ms, max_rss_kb
FROM submissions
WHERE submission_id = :'second_submission_id';

SELECT status::text, score, compile_output, judge_output, elapsed_ms, max_rss_kb
FROM submissions
WHERE submission_id = :'third_submission_id';

SELECT accepted_count
FROM problem_statistics
WHERE problem_id = :'problem_id';

SELECT priority
FROM submission_queue
WHERE submission_id = :'first_submission_id';

SELECT priority
FROM submission_queue
WHERE submission_id = :'second_submission_id';

SELECT COUNT(*)
FROM submission_queue
WHERE submission_id IN (
    :'first_submission_id',
    :'second_submission_id',
    :'third_submission_id'
);
SQL
)

if [[ "${#problem_rejudge_values[@]}" -ne 7 ]]; then
    append_log_line "${test_log_temp_file}" "problem rejudge db validation failed: unexpected row count"
    publish_failure_logs
    echo "problem rejudge db validation failed: expected 7 rows, got ${#problem_rejudge_values[@]}" >&2
    exit 1
fi

if [[ "${problem_rejudge_values[0]}" != "queued|||||" ]]; then
    append_log_line "${test_log_temp_file}" "problem rejudge db validation failed: first_submission=${problem_rejudge_values[0]}"
    publish_failure_logs
    echo "problem rejudge db validation failed for first submission: ${problem_rejudge_values[0]}" >&2
    exit 1
fi

if [[ "${problem_rejudge_values[1]}" != "queued|||||" ]]; then
    append_log_line "${test_log_temp_file}" "problem rejudge db validation failed: second_submission=${problem_rejudge_values[1]}"
    publish_failure_logs
    echo "problem rejudge db validation failed for second submission: ${problem_rejudge_values[1]}" >&2
    exit 1
fi

if [[ "${problem_rejudge_values[2]}" != "compile_error|0|compile error stale compile output|compile error stale judge output||" ]]; then
    append_log_line "${test_log_temp_file}" "problem rejudge db validation failed: third_submission=${problem_rejudge_values[2]}"
    publish_failure_logs
    echo "problem rejudge db validation failed for third submission: ${problem_rejudge_values[2]}" >&2
    exit 1
fi

if [[ "${problem_rejudge_values[3]}" != "0" ]]; then
    append_log_line "${test_log_temp_file}" "problem rejudge db validation failed: accepted_count=${problem_rejudge_values[3]}"
    publish_failure_logs
    echo "problem rejudge db validation failed: expected accepted_count 0, got ${problem_rejudge_values[3]}" >&2
    exit 1
fi

if [[ "${problem_rejudge_values[4]}" != "0" ]]; then
    append_log_line "${test_log_temp_file}" "problem rejudge db validation failed: first_queue_priority=${problem_rejudge_values[4]}"
    publish_failure_logs
    echo "problem rejudge db validation failed: expected first queue priority 0, got ${problem_rejudge_values[4]}" >&2
    exit 1
fi

if [[ "${problem_rejudge_values[5]}" != "0" ]]; then
    append_log_line "${test_log_temp_file}" "problem rejudge db validation failed: second_queue_priority=${problem_rejudge_values[5]}"
    publish_failure_logs
    echo "problem rejudge db validation failed: expected second queue priority 0, got ${problem_rejudge_values[5]}" >&2
    exit 1
fi

if [[ "${problem_rejudge_values[6]}" != "2" ]]; then
    append_log_line "${test_log_temp_file}" "problem rejudge db validation failed: queue_count=${problem_rejudge_values[6]}"
    publish_failure_logs
    echo "problem rejudge db validation failed: expected queue_count 2, got ${problem_rejudge_values[6]}" >&2
    exit 1
fi

print_success_log "problem rejudge success"

append_log_line "${test_log_temp_file}" "submission flow test passed"
print_success_log "submission flow test passed: login_id=${user_login_id}, user_id=${sign_up_user_id}, problem_id=${problem_id}, submission_id=${submission_id}, second_submission_id=${second_submission_id}"
