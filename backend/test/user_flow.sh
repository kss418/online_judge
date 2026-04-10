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

make_submission_ban_request_body(){
    local duration_minutes="$1"

    python3 - "${duration_minutes}" <<'PY'
import json
import sys

print(json.dumps({"duration_minutes": int(sys.argv[1])}))
PY
}

assert_user_summary_response(){
    local response_file_path="$1"
    local expected_user_id="$2"
    local expected_user_login_id="$3"
    local failure_context="$4"

    if ! python3 \
        - "${response_file_path}" "${expected_user_id}" "${expected_user_login_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("user_id") != int(sys.argv[2]):
    raise SystemExit("user summary id mismatch")

if response.get("user_login_id") != sys.argv[3]:
    raise SystemExit("user summary login id mismatch")

created_at = response.get("created_at")
if not isinstance(created_at, str) or not created_at:
    raise SystemExit("user summary created_at missing")

for field_name in ("permission_level", "role_name", "user_name", "is_admin"):
    if field_name in response:
        raise SystemExit(f"did not expect {field_name} in user summary")
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} validation failed"
        publish_failure_logs
        exit 1
    fi
}

assert_public_user_list_response(){
    local response_file_path="$1"
    local expected_user_id="$2"
    local expected_user_login_id="$3"
    local failure_context="$4"

    if ! python3 \
        - "${response_file_path}" "${expected_user_id}" "${expected_user_login_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("user_count") != 1:
    raise SystemExit("expected one public user in filtered list")

users = response.get("users")
if not isinstance(users, list) or len(users) != 1:
    raise SystemExit("public user list users mismatch")

user_value = users[0]
if user_value.get("user_id") != int(sys.argv[2]):
    raise SystemExit("public user list id mismatch")

if user_value.get("user_login_id") != sys.argv[3]:
    raise SystemExit("public user list login id mismatch")

if user_value.get("solved_problem_count") != 0:
    raise SystemExit("expected solved_problem_count to be 0")

if user_value.get("accepted_submission_count") != 0:
    raise SystemExit("expected accepted_submission_count to be 0")

if user_value.get("submission_count") != 0:
    raise SystemExit("expected submission_count to be 0")

created_at = user_value.get("created_at")
if not isinstance(created_at, str) or not created_at:
    raise SystemExit("public user list created_at missing")
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} validation failed"
        publish_failure_logs
        exit 1
    fi
}

assert_empty_statistics_response(){
    local response_file_path="$1"
    local expected_user_id="$2"
    local failure_context="$3"

    if ! python3 - "${response_file_path}" "${expected_user_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("user_id") != int(sys.argv[2]):
    raise SystemExit("statistics user_id mismatch")

expected_zero_fields = [
    "submission_count",
    "queued_submission_count",
    "judging_submission_count",
    "accepted_submission_count",
    "wrong_answer_submission_count",
    "time_limit_exceeded_submission_count",
    "memory_limit_exceeded_submission_count",
    "runtime_error_submission_count",
    "compile_error_submission_count",
    "build_resource_exceeded_submission_count",
    "output_exceeded_submission_count",
    "infra_failure_submission_count",
]

for field_name in expected_zero_fields:
    if response.get(field_name) != 0:
        raise SystemExit(f"expected {field_name} to be 0")

if response.get("last_submission_at") is not None:
    raise SystemExit("expected last_submission_at to be null")

if response.get("last_accepted_at") is not None:
    raise SystemExit("expected last_accepted_at to be null")

updated_at = response.get("updated_at")
if not isinstance(updated_at, str) or not updated_at:
    raise SystemExit("updated_at missing")
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} validation failed"
        publish_failure_logs
        exit 1
    fi
}

assert_empty_problem_list_response(){
    local response_file_path="$1"
    local count_field="$2"
    local list_field="$3"
    local failure_context="$4"

    if ! python3 - "${response_file_path}" "${count_field}" "${list_field}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

count_field = sys.argv[2]
list_field = sys.argv[3]

if response.get(count_field) != 0:
    raise SystemExit(f"expected {count_field} to be 0")

if response.get(list_field) != []:
    raise SystemExit(f"expected {list_field} to be an empty array")
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} validation failed"
        publish_failure_logs
        exit 1
    fi
}

assert_me_response(){
    local response_file_path="$1"
    local expected_user_id="$2"
    local expected_user_login_id="$3"
    local expected_permission_level="$4"
    local expected_role_name="$5"
    local failure_context="$6"

    if ! python3 \
        - "${response_file_path}" \
        "${expected_user_id}" \
        "${expected_user_login_id}" \
        "${expected_permission_level}" \
        "${expected_role_name}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("id") != int(sys.argv[2]):
    raise SystemExit("me response id mismatch")

if response.get("user_login_id") != sys.argv[3]:
    raise SystemExit("me response login id mismatch")

if response.get("permission_level") != int(sys.argv[4]):
    raise SystemExit("me response permission level mismatch")

if response.get("role_name") != sys.argv[5]:
    raise SystemExit("me response role_name mismatch")

if "is_admin" in response:
    raise SystemExit("did not expect is_admin in me response")
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} validation failed"
        publish_failure_logs
        exit 1
    fi
}

assert_admin_user_list_response(){
    local response_file_path="$1"
    local superadmin_user_id="$2"
    local superadmin_user_login_id="$3"
    local target_user_id="$4"
    local target_user_login_id="$5"
    local admin_user_id="$6"
    local admin_user_login_id="$7"
    local failure_context="$8"

    if ! python3 \
        - "${response_file_path}" \
        "${superadmin_user_id}" \
        "${superadmin_user_login_id}" \
        "${target_user_id}" \
        "${target_user_login_id}" \
        "${admin_user_id}" \
        "${admin_user_login_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

users = response.get("users")
user_count = response.get("user_count")
if not isinstance(users, list):
    raise SystemExit("admin user list users is not a list")
if not isinstance(user_count, int):
    raise SystemExit("admin user list user_count is not an int")
if user_count != len(users):
    raise SystemExit("admin user list user_count mismatch")
if user_count < 3:
    raise SystemExit("expected at least three users in admin user list")

expected_users = {
    sys.argv[3]: (int(sys.argv[2]), 2, "superadmin"),
    sys.argv[5]: (int(sys.argv[4]), 0, "user"),
    sys.argv[7]: (int(sys.argv[6]), 1, "admin"),
}

indexed_users = {user.get("user_login_id"): user for user in users}

for user_login_id, (user_id, permission_level, role_name) in expected_users.items():
    if user_login_id not in indexed_users:
        raise SystemExit(f"missing user in admin list: {user_login_id}")

    user_value = indexed_users[user_login_id]
    if user_value.get("user_id") != user_id:
        raise SystemExit(f"user_id mismatch for {user_login_id}")
    if user_value.get("permission_level") != permission_level:
        raise SystemExit(f"permission_level mismatch for {user_login_id}")
    if user_value.get("role_name") != role_name:
        raise SystemExit(f"role_name mismatch for {user_login_id}")

    created_at = user_value.get("created_at")
    if not isinstance(created_at, str) or not created_at:
        raise SystemExit(f"created_at missing for {user_login_id}")
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} validation failed"
        publish_failure_logs
        exit 1
    fi
}

http_port="${USER_FLOW_TEST_HTTP_PORT:-18080}"
base_url="${USER_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${USER_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
superadmin_user_login_id="${USER_FLOW_TEST_SUPERADMIN_LOGIN_ID:-$(make_test_login_id ua)}"
target_user_login_id="${USER_FLOW_TEST_TARGET_LOGIN_ID:-$(make_test_login_id ub)}"
admin_user_login_id="${USER_FLOW_TEST_ADMIN_LOGIN_ID:-$(make_test_login_id uc)}"
raw_password="${USER_FLOW_TEST_PASSWORD:-password123}"
test_db_name="user_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_user_flow.log"
server_log_name="test_user_flow_server.log"

init_flow_test
register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file superadmin_sign_up_response_file
register_temp_file target_user_sign_up_response_file
register_temp_file admin_user_sign_up_response_file
register_temp_file superadmin_login_response_file
register_temp_file admin_login_response_file
register_temp_file public_user_summary_by_login_response_file
register_temp_file public_user_summary_response_file
register_temp_file public_user_list_response_file
register_temp_file public_user_statistics_response_file
register_temp_file public_user_solved_problems_response_file
register_temp_file public_user_wrong_problems_response_file
register_temp_file user_me_response_file
register_temp_file user_me_statistics_response_file
register_temp_file user_me_solved_problems_response_file
register_temp_file user_me_wrong_problems_response_file
register_temp_file user_me_submission_ban_response_file
register_temp_file promote_admin_response_file
register_temp_file admin_user_list_response_file
register_temp_file create_submission_ban_response_file
register_temp_file get_submission_ban_response_file
register_temp_file clear_submission_ban_response_file
register_temp_file cleared_submission_ban_response_file
register_temp_file cleared_me_submission_ban_response_file
register_temp_file demote_admin_response_file

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
append_log_line "${test_log_temp_file}" "superadmin_user_login_id=${superadmin_user_login_id}"
append_log_line "${test_log_temp_file}" "target_user_login_id=${target_user_login_id}"
append_log_line "${test_log_temp_file}" "admin_user_login_id=${admin_user_login_id}"
append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"

apply_test_database_migrations
ensure_dedicated_http_server

read -r superadmin_user_id superadmin_user_token < <(
    sign_up_user \
        "${superadmin_user_login_id}" \
        "${raw_password}" \
        "${superadmin_sign_up_response_file}" \
        "user flow superadmin"
)
print_success_log "superadmin candidate sign-up success"

read -r target_user_id target_user_token < <(
    sign_up_user \
        "${target_user_login_id}" \
        "${raw_password}" \
        "${target_user_sign_up_response_file}" \
        "user flow target"
)
print_success_log "target user sign-up success"

read -r admin_candidate_user_id admin_candidate_user_token < <(
    sign_up_user \
        "${admin_user_login_id}" \
        "${raw_password}" \
        "${admin_user_sign_up_response_file}" \
        "user flow admin candidate"
)
print_success_log "admin candidate sign-up success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/id/${target_user_login_id}" \
    "${public_user_summary_by_login_response_file}" \
    "200" \
    "get public user summary by login id"
assert_user_summary_response \
    "${public_user_summary_by_login_response_file}" \
    "${target_user_id}" \
    "${target_user_login_id}" \
    "get public user summary by login id"
print_success_log "public user summary by login id get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/${target_user_id}" \
    "${public_user_summary_response_file}" \
    "200" \
    "get public user summary by id"
assert_user_summary_response \
    "${public_user_summary_response_file}" \
    "${target_user_id}" \
    "${target_user_login_id}" \
    "get public user summary by id"
print_success_log "public user summary by id get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/list?q=${target_user_login_id}" \
    "${public_user_list_response_file}" \
    "200" \
    "get public user list"
assert_public_user_list_response \
    "${public_user_list_response_file}" \
    "${target_user_id}" \
    "${target_user_login_id}" \
    "get public user list"
print_success_log "public user list get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/${target_user_id}/statistics" \
    "${public_user_statistics_response_file}" \
    "200" \
    "get public user statistics"
assert_empty_statistics_response \
    "${public_user_statistics_response_file}" \
    "${target_user_id}" \
    "get public user statistics"
print_success_log "public user statistics get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/${target_user_id}/solved-problems" \
    "${public_user_solved_problems_response_file}" \
    "200" \
    "get public user solved problems"
assert_empty_problem_list_response \
    "${public_user_solved_problems_response_file}" \
    "solved_problem_count" \
    "solved_problems" \
    "get public user solved problems"
print_success_log "public user solved problems get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/${target_user_id}/wrong-problems" \
    "${public_user_wrong_problems_response_file}" \
    "200" \
    "get public user wrong problems"
assert_empty_problem_list_response \
    "${public_user_wrong_problems_response_file}" \
    "wrong_problem_count" \
    "wrong_problems" \
    "get public user wrong problems"
print_success_log "public user wrong problems get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me" \
    "${user_me_response_file}" \
    "200" \
    "get current user" \
    "${target_user_token}"
assert_me_response \
    "${user_me_response_file}" \
    "${target_user_id}" \
    "${target_user_login_id}" \
    "0" \
    "user" \
    "get current user"
print_success_log "current user get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me/statistics" \
    "${user_me_statistics_response_file}" \
    "200" \
    "get current user statistics" \
    "${target_user_token}"
assert_empty_statistics_response \
    "${user_me_statistics_response_file}" \
    "${target_user_id}" \
    "get current user statistics"
print_success_log "current user statistics get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me/solved-problems" \
    "${user_me_solved_problems_response_file}" \
    "200" \
    "get current user solved problems" \
    "${target_user_token}"
assert_empty_problem_list_response \
    "${user_me_solved_problems_response_file}" \
    "solved_problem_count" \
    "solved_problems" \
    "get current user solved problems"
print_success_log "current user solved problems get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me/wrong-problems" \
    "${user_me_wrong_problems_response_file}" \
    "200" \
    "get current user wrong problems" \
    "${target_user_token}"
assert_empty_problem_list_response \
    "${user_me_wrong_problems_response_file}" \
    "wrong_problem_count" \
    "wrong_problems" \
    "get current user wrong problems"
print_success_log "current user wrong problems get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me/submission-ban" \
    "${user_me_submission_ban_response_file}" \
    "200" \
    "get current user submission ban before create" \
    "${target_user_token}"
assert_json_field_equals \
    "${user_me_submission_ban_response_file}" \
    "user_id" \
    "${target_user_id}" \
    "get current user submission ban before create" \
    "int"
assert_json_field_equals \
    "${user_me_submission_ban_response_file}" \
    "submission_banned_until" \
    "" \
    "get current user submission ban before create" \
    "null"
print_success_log "current user submission ban get success before create"

promote_superadmin_user "${superadmin_user_id}" "user flow" >/dev/null
print_success_log "superadmin bootstrap promote success"

superadmin_login_request_body="$(
    make_login_request_body "${superadmin_user_login_id}" "${raw_password}"
)"
send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/auth/login" \
    "${superadmin_login_response_file}" \
    "200" \
    "superadmin login" \
    "" \
    "${superadmin_login_request_body}"
superadmin_user_token="$(read_json_field "${superadmin_login_response_file}" "token" "string")"
print_success_log "superadmin login success"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/user/${admin_candidate_user_id}/admin" \
    "${promote_admin_response_file}" \
    "200" \
    "promote admin" \
    "${superadmin_user_token}"
assert_json_field_equals \
    "${promote_admin_response_file}" \
    "user_id" \
    "${admin_candidate_user_id}" \
    "promote admin" \
    "int"
assert_json_field_equals \
    "${promote_admin_response_file}" \
    "permission_level" \
    "1" \
    "promote admin" \
    "int"
assert_json_field_equals \
    "${promote_admin_response_file}" \
    "role_name" \
    "admin" \
    "promote admin"
print_success_log "admin promote success"

admin_login_request_body="$(
    make_login_request_body "${admin_user_login_id}" "${raw_password}"
)"
send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/auth/login" \
    "${admin_login_response_file}" \
    "200" \
    "admin login" \
    "" \
    "${admin_login_request_body}"
admin_candidate_user_token="$(read_json_field "${admin_login_response_file}" "token" "string")"
print_success_log "admin login success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user" \
    "${admin_user_list_response_file}" \
    "200" \
    "get admin user list" \
    "${admin_candidate_user_token}"
assert_admin_user_list_response \
    "${admin_user_list_response_file}" \
    "${superadmin_user_id}" \
    "${superadmin_user_login_id}" \
    "${target_user_id}" \
    "${target_user_login_id}" \
    "${admin_candidate_user_id}" \
    "${admin_user_login_id}" \
    "get admin user list"
print_success_log "admin user list get success"

submission_ban_request_body="$(
    make_submission_ban_request_body "60"
)"
send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/user/${target_user_id}/submission-ban" \
    "${create_submission_ban_response_file}" \
    "201" \
    "create submission ban" \
    "${admin_candidate_user_token}" \
    "${submission_ban_request_body}"
assert_json_field_equals \
    "${create_submission_ban_response_file}" \
    "user_id" \
    "${target_user_id}" \
    "create submission ban" \
    "int"
assert_json_field_equals \
    "${create_submission_ban_response_file}" \
    "duration_minutes" \
    "60" \
    "create submission ban" \
    "int"
submission_banned_until="$(
    read_json_field \
        "${create_submission_ban_response_file}" \
        "submission_banned_until" \
        "string"
)"
print_success_log "submission ban create success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/${target_user_id}/submission-ban" \
    "${get_submission_ban_response_file}" \
    "200" \
    "get submission ban" \
    "${admin_candidate_user_token}"
assert_json_field_equals \
    "${get_submission_ban_response_file}" \
    "user_id" \
    "${target_user_id}" \
    "get submission ban" \
    "int"
assert_json_field_equals \
    "${get_submission_ban_response_file}" \
    "submission_banned_until" \
    "${submission_banned_until}" \
    "get submission ban"
print_success_log "submission ban get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me/submission-ban" \
    "${user_me_submission_ban_response_file}" \
    "200" \
    "get current user submission ban after create" \
    "${target_user_token}"
assert_json_field_equals \
    "${user_me_submission_ban_response_file}" \
    "user_id" \
    "${target_user_id}" \
    "get current user submission ban after create" \
    "int"
assert_json_field_equals \
    "${user_me_submission_ban_response_file}" \
    "submission_banned_until" \
    "${submission_banned_until}" \
    "get current user submission ban after create"
print_success_log "current user submission ban get success after create"

send_http_request_and_assert_status \
    "DELETE" \
    "${base_url}/api/user/${target_user_id}/submission-ban" \
    "${clear_submission_ban_response_file}" \
    "200" \
    "clear submission ban" \
    "${admin_candidate_user_token}"
assert_json_message \
    "${clear_submission_ban_response_file}" \
    "user submission ban cleared" \
    "clear submission ban"
print_success_log "submission ban clear success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/${target_user_id}/submission-ban" \
    "${cleared_submission_ban_response_file}" \
    "200" \
    "get cleared submission ban" \
    "${admin_candidate_user_token}"
assert_json_field_equals \
    "${cleared_submission_ban_response_file}" \
    "user_id" \
    "${target_user_id}" \
    "get cleared submission ban" \
    "int"
assert_json_field_equals \
    "${cleared_submission_ban_response_file}" \
    "submission_banned_until" \
    "" \
    "get cleared submission ban" \
    "null"
print_success_log "submission ban cleared status get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me/submission-ban" \
    "${cleared_me_submission_ban_response_file}" \
    "200" \
    "get current user submission ban after clear" \
    "${target_user_token}"
assert_json_field_equals \
    "${cleared_me_submission_ban_response_file}" \
    "user_id" \
    "${target_user_id}" \
    "get current user submission ban after clear" \
    "int"
assert_json_field_equals \
    "${cleared_me_submission_ban_response_file}" \
    "submission_banned_until" \
    "" \
    "get current user submission ban after clear" \
    "null"
print_success_log "current user submission ban get success after clear"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/user/${admin_candidate_user_id}/user" \
    "${demote_admin_response_file}" \
    "200" \
    "demote admin to user" \
    "${superadmin_user_token}"
assert_json_field_equals \
    "${demote_admin_response_file}" \
    "user_id" \
    "${admin_candidate_user_id}" \
    "demote admin to user" \
    "int"
assert_json_field_equals \
    "${demote_admin_response_file}" \
    "permission_level" \
    "0" \
    "demote admin to user" \
    "int"
assert_json_field_equals \
    "${demote_admin_response_file}" \
    "role_name" \
    "user" \
    "demote admin to user"
print_success_log "admin demote success"

print_success_log "user flow test passed"
