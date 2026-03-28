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

http_port="${AUTH_FLOW_TEST_HTTP_PORT:-18080}"
base_url="${AUTH_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${AUTH_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${AUTH_FLOW_TEST_LOGIN_ID:-$(make_test_login_id af)}"
second_user_login_id="${AUTH_FLOW_TEST_SECOND_LOGIN_ID:-$(make_test_login_id af s)}"
duplicate_user_login_id="${AUTH_FLOW_TEST_DUPLICATE_LOGIN_ID:-${user_login_id}}"
raw_password="${AUTH_FLOW_TEST_PASSWORD:-password123}"
test_db_name="auth_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_auth_flow.log"
server_log_name="test_auth_flow_server.log"

init_flow_test
register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file sign_up_response_file
register_temp_file login_response_file
register_temp_file renew_response_file
register_temp_file logout_response_file
register_temp_file second_logout_response_file
register_temp_file second_sign_up_response_file
register_temp_file second_login_response_file
register_temp_file third_login_response_file
register_temp_file user_me_response_file
register_temp_file user_summary_response_file
register_temp_file user_public_list_response_file
register_temp_file user_statistics_response_file
register_temp_file user_solved_problems_response_file
register_temp_file user_wrong_problems_response_file
register_temp_file user_me_statistics_response_file
register_temp_file user_me_solved_problems_response_file
register_temp_file user_me_wrong_problems_response_file
register_temp_file promote_admin_response_file
register_temp_file demote_user_response_file
register_temp_file unauthorized_promote_response_file
register_temp_file unauthorized_demote_response_file
register_temp_file duplicate_user_login_id_response_file

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

sign_up_user \
    "${user_login_id}" \
    "${raw_password}" \
    "${sign_up_response_file}" \
    "auth flow" >/dev/null
print_success_log "sign-up success"

request_body="$(make_login_request_body "${user_login_id}" "${raw_password}")"

send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/auth/login" \
    "${login_response_file}" \
    "200" \
    "login" \
    "" \
    "${request_body}"
print_success_log "login success"

if ! python3 - "${sign_up_response_file}" "${login_response_file}" "${user_login_id}" <<'PY'
import json
import sys

sign_up_response_file_path = sys.argv[1]
login_response_file_path = sys.argv[2]
expected_user_login_id = sys.argv[3]

with open(sign_up_response_file_path, encoding="utf-8") as response_file:
    sign_up_response = json.load(response_file)

with open(login_response_file_path, encoding="utf-8") as response_file:
    login_response = json.load(response_file)

sign_up_user_id = sign_up_response.get("user_id")
sign_up_permission_level = sign_up_response.get("permission_level")
sign_up_role_name = sign_up_response.get("role_name")
sign_up_user_login_id = sign_up_response.get("user_login_id")
sign_up_token = sign_up_response.get("token")

login_user_id = login_response.get("user_id")
login_permission_level = login_response.get("permission_level")
login_role_name = login_response.get("role_name")
login_user_login_id = login_response.get("user_login_id")
login_token = login_response.get("token")

if not isinstance(sign_up_user_id, int) or sign_up_user_id <= 0:
    raise SystemExit("invalid user_id in sign-up response")

if sign_up_permission_level != 0:
    raise SystemExit("expected permission_level to be 0 for new user")

if sign_up_role_name != "user":
    raise SystemExit("expected role_name to be user for new user")

if sign_up_user_login_id != expected_user_login_id:
    raise SystemExit("sign-up response user_login_id mismatch")

if not isinstance(sign_up_token, str) or not sign_up_token:
    raise SystemExit("missing token in sign-up response")

if login_user_id != sign_up_user_id:
    raise SystemExit("login response user_id does not match sign-up response")

if login_permission_level != 0:
    raise SystemExit("expected permission_level to stay 0 after login")

if login_role_name != "user":
    raise SystemExit("expected role_name to stay user after login")

if login_user_login_id != expected_user_login_id:
    raise SystemExit("login response user_login_id mismatch")

if not isinstance(login_token, str) or not login_token:
    raise SystemExit("missing token in login response")

if login_token == sign_up_token:
    raise SystemExit("expected login to issue a new token")
PY
then
    append_log_line "${test_log_temp_file}" "response validation failed"
    publish_failure_logs
    exit 1
fi

append_log_line "${test_log_temp_file}" "auth flow test passed"

login_user_id="$(read_json_field "${login_response_file}" "user_id" "int")"
login_token="$(read_json_field "${login_response_file}" "token" "string")"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/id/${user_login_id}" \
    "${user_summary_response_file}" \
    "200" \
    "get public user summary"
if ! python3 - "${user_summary_response_file}" "${login_user_id}" "${user_login_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    user_summary = json.load(response_file)

expected_user_id = int(sys.argv[2])
expected_user_login_id = sys.argv[3]

if user_summary.get("user_id") != expected_user_id:
    raise SystemExit("public user summary id mismatch")

if user_summary.get("user_login_id") != expected_user_login_id:
    raise SystemExit("public user summary user_login_id mismatch")

created_at = user_summary.get("created_at")
if not isinstance(created_at, str) or not created_at:
    raise SystemExit("public user summary created_at missing")

if "permission_level" in user_summary:
    raise SystemExit("did not expect permission_level in public user summary")

if "role_name" in user_summary:
    raise SystemExit("did not expect role_name in public user summary")

if "user_name" in user_summary:
    raise SystemExit("did not expect user_name in public user summary")
PY
then
    append_log_line "${test_log_temp_file}" "public user summary validation failed"
    publish_failure_logs
    exit 1
fi
print_success_log "public user summary get success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/list?q=${user_login_id}" \
    "${user_public_list_response_file}" \
    "200" \
    "get public user list before submissions"
if ! python3 - "${user_public_list_response_file}" "${login_user_id}" "${user_login_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    user_list = json.load(response_file)

expected_user_id = int(sys.argv[2])
expected_user_login_id = sys.argv[3]

if user_list.get("user_count") != 1:
    raise SystemExit("expected one user in filtered public user list")

users = user_list.get("users")
if not isinstance(users, list) or len(users) != 1:
    raise SystemExit("public user list users mismatch")

user_value = users[0]
if user_value.get("user_id") != expected_user_id:
    raise SystemExit("public user list user_id mismatch")

if user_value.get("user_login_id") != expected_user_login_id:
    raise SystemExit("public user list user_login_id mismatch")

if user_value.get("solved_problem_count") != 0:
    raise SystemExit("expected solved_problem_count to be 0 before submissions")

if user_value.get("accepted_submission_count") != 0:
    raise SystemExit("expected accepted_submission_count to be 0 before submissions")

if user_value.get("submission_count") != 0:
    raise SystemExit("expected submission_count to be 0 before submissions")

created_at = user_value.get("created_at")
if not isinstance(created_at, str) or not created_at:
    raise SystemExit("public user list created_at missing")
PY
then
    append_log_line "${test_log_temp_file}" "public user list validation failed"
    publish_failure_logs
    exit 1
fi
print_success_log "public user list get success before submissions"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/${login_user_id}/statistics" \
    "${user_statistics_response_file}" \
    "200" \
    "get public user statistics before submissions"
if ! python3 - "${user_statistics_response_file}" "${login_user_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    statistics = json.load(response_file)

expected_user_id = int(sys.argv[2])

if statistics.get("user_id") != expected_user_id:
    raise SystemExit("public user statistics id mismatch")

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
    "output_exceeded_submission_count",
]
for field in expected_zero_fields:
    if statistics.get(field) != 0:
        raise SystemExit(f"expected {field} to be 0")

if statistics.get("last_submission_at") is not None:
    raise SystemExit("expected last_submission_at to be null")

if statistics.get("last_accepted_at") is not None:
    raise SystemExit("expected last_accepted_at to be null")

updated_at = statistics.get("updated_at")
if not isinstance(updated_at, str) or not updated_at:
    raise SystemExit("expected updated_at to be present")
PY
then
    append_log_line "${test_log_temp_file}" "public user statistics validation failed"
    publish_failure_logs
    exit 1
fi
print_success_log "public user statistics get success before submissions"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/${login_user_id}/solved-problems" \
    "${user_solved_problems_response_file}" \
    "200" \
    "get public user solved problems before submissions"
if ! python3 - "${user_solved_problems_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    solved = json.load(response_file)

if solved.get("solved_problem_count") != 0:
    raise SystemExit("expected solved_problem_count to be 0")

if solved.get("solved_problems") != []:
    raise SystemExit("expected solved_problems to be empty")
PY
then
    append_log_line "${test_log_temp_file}" "public user solved problems validation failed"
    publish_failure_logs
    exit 1
fi
print_success_log "public user solved problems get success before submissions"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/${login_user_id}/wrong-problems" \
    "${user_wrong_problems_response_file}" \
    "200" \
    "get public user wrong problems before submissions"
if ! python3 - "${user_wrong_problems_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    wrong = json.load(response_file)

if wrong.get("wrong_problem_count") != 0:
    raise SystemExit("expected wrong_problem_count to be 0")

if wrong.get("wrong_problems") != []:
    raise SystemExit("expected wrong_problems to be empty")
PY
then
    append_log_line "${test_log_temp_file}" "public user wrong problems validation failed"
    publish_failure_logs
    exit 1
fi
print_success_log "public user wrong problems get success before submissions"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me" \
    "${user_me_response_file}" \
    "200" \
    "get current user before promote" \
    "${login_token}"
if ! python3 - "${user_me_response_file}" "${login_user_id}" "${user_login_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    current_user = json.load(response_file)

expected_user_id = int(sys.argv[2])
expected_user_login_id = sys.argv[3]

if current_user.get("id") != expected_user_id:
    raise SystemExit("current user id mismatch before promote")

if current_user.get("user_login_id") != expected_user_login_id:
    raise SystemExit("current user user_login_id mismatch before promote")

if current_user.get("permission_level") != 0:
    raise SystemExit("expected current user permission_level to be 0 before promote")

if current_user.get("role_name") != "user":
    raise SystemExit("expected current user role_name to be user before promote")

if "is_admin" in current_user:
    raise SystemExit("did not expect is_admin in current user response")
PY
then
    append_log_line "${test_log_temp_file}" "current user validation failed before promote"
    publish_failure_logs
    exit 1
fi
print_success_log "current user get success before promote"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me/statistics" \
    "${user_me_statistics_response_file}" \
    "200" \
    "get current user statistics before submissions" \
    "${login_token}"
if ! python3 - "${user_me_statistics_response_file}" "${login_user_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    statistics = json.load(response_file)

expected_user_id = int(sys.argv[2])
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
    "output_exceeded_submission_count",
]

if statistics.get("user_id") != expected_user_id:
    raise SystemExit("current user statistics user_id mismatch")

for field_name in expected_zero_fields:
    if statistics.get(field_name) != 0:
        raise SystemExit(f"expected {field_name} to be 0 before submissions")

if statistics.get("last_submission_at") is not None:
    raise SystemExit("expected last_submission_at to be null before submissions")

if statistics.get("last_accepted_at") is not None:
    raise SystemExit("expected last_accepted_at to be null before submissions")

updated_at = statistics.get("updated_at")
if not isinstance(updated_at, str) or not updated_at:
    raise SystemExit("expected updated_at to be a non-empty string")
PY
then
    append_log_line "${test_log_temp_file}" "current user statistics validation failed before submissions"
    publish_failure_logs
    exit 1
fi
print_success_log "current user statistics get success before submissions"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me/solved-problems" \
    "${user_me_solved_problems_response_file}" \
    "200" \
    "get current user solved problems before submissions" \
    "${login_token}"
if ! python3 - "${user_me_solved_problems_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    solved_problems = json.load(response_file)

if solved_problems.get("solved_problem_count") != 0:
    raise SystemExit("expected solved_problem_count to be 0 before submissions")

solved_problem_values = solved_problems.get("solved_problems")
if solved_problem_values != []:
    raise SystemExit("expected solved_problems to be an empty array before submissions")
PY
then
    append_log_line "${test_log_temp_file}" "current user solved problems validation failed before submissions"
    publish_failure_logs
    exit 1
fi
print_success_log "current user solved problems get success before submissions"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me/wrong-problems" \
    "${user_me_wrong_problems_response_file}" \
    "200" \
    "get current user wrong problems before submissions" \
    "${login_token}"
if ! python3 - "${user_me_wrong_problems_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    wrong_problems = json.load(response_file)

if wrong_problems.get("wrong_problem_count") != 0:
    raise SystemExit("expected wrong_problem_count to be 0 before submissions")

wrong_problem_values = wrong_problems.get("wrong_problems")
if wrong_problem_values != []:
    raise SystemExit("expected wrong_problems to be an empty array before submissions")
PY
then
    append_log_line "${test_log_temp_file}" "current user wrong problems validation failed before submissions"
    publish_failure_logs
    exit 1
fi
print_success_log "current user wrong problems get success before submissions"

duplicate_user_login_id_request_body="$(
    make_sign_up_request_body \
        "${duplicate_user_login_id}" \
        "${raw_password}"
)"
send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/auth/sign-up" \
    "${duplicate_user_login_id_response_file}" \
    "409" \
    "duplicate user_login_id sign-up" \
    "" \
    "${duplicate_user_login_id_request_body}"
assert_json_error_code \
    "${duplicate_user_login_id_response_file}" \
    "conflict" \
    "duplicate user_login_id sign-up"
assert_json_error_message \
    "${duplicate_user_login_id_response_file}" \
    "failed to sign up: psql unique violation" \
    "duplicate user_login_id sign-up"
print_success_log "duplicate user_login_id failure success"

read -r second_user_id second_user_token < <(
    sign_up_user \
        "${second_user_login_id}" \
        "${raw_password}" \
        "${second_sign_up_response_file}" \
        "auth flow"
)
print_success_log "second sign-up success"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/user/${login_user_id}/admin" \
    "${unauthorized_promote_response_file}" \
    "401" \
    "unauthorized promote" \
    "${second_user_token}"
print_success_log "unauthorized promote success"
assert_json_error_code \
    "${unauthorized_promote_response_file}" \
    "superadmin_bearer_token_required" \
    "unauthorized promote"

promote_superadmin_user "${login_user_id}" "auth flow" >/dev/null
print_success_log "bootstrap superadmin promote success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/list?q=${user_login_id}" \
    "${user_public_list_response_file}" \
    "200" \
    "get public user list after superadmin promote"
if ! python3 - "${user_public_list_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    user_list = json.load(response_file)

if user_list.get("user_count") != 0:
    raise SystemExit("expected promoted superadmin to be hidden from public user list")

users = user_list.get("users")
if not isinstance(users, list) or users:
    raise SystemExit("expected no public users in filtered list for superadmin")
PY
then
    append_log_line "${test_log_temp_file}" "public user list superadmin filtering failed"
    publish_failure_logs
    exit 1
fi
print_success_log "public user list hides superadmin success"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/user/${second_user_id}/admin" \
    "${promote_admin_response_file}" \
    "200" \
    "promote admin" \
    "${login_token}"
print_success_log "promote admin success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/user/me" \
    "${user_me_response_file}" \
    "200" \
    "get current user after promote" \
    "${login_token}"
if ! python3 - "${user_me_response_file}" "${login_user_id}" "${user_login_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    current_user = json.load(response_file)

expected_user_id = int(sys.argv[2])
expected_user_login_id = sys.argv[3]

if current_user.get("id") != expected_user_id:
    raise SystemExit("current user id mismatch after promote")

if current_user.get("user_login_id") != expected_user_login_id:
    raise SystemExit("current user user_login_id mismatch after promote")

if current_user.get("permission_level") != 2:
    raise SystemExit("expected current user permission_level to be 2 after promote")

if current_user.get("role_name") != "superadmin":
    raise SystemExit("expected current user role_name to be superadmin after promote")

if "is_admin" in current_user:
    raise SystemExit("did not expect is_admin in current user response")
PY
then
    append_log_line "${test_log_temp_file}" "current user validation failed after promote"
    publish_failure_logs
    exit 1
fi
print_success_log "current user get success after promote"

second_login_request_body="$(make_login_request_body "${second_user_login_id}" "${raw_password}")"
send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/auth/login" \
    "${second_login_response_file}" \
    "200" \
    "second login" \
    "" \
    "${second_login_request_body}"
print_success_log "second login success"

if ! python3 \
    - "${unauthorized_promote_response_file}" \
    "${promote_admin_response_file}" \
    "${second_login_response_file}" \
    "${second_user_id}" \
    "${second_user_login_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    unauthorized_promote_response = json.load(response_file)

with open(sys.argv[2], encoding="utf-8") as response_file:
    promote_admin_response = json.load(response_file)

with open(sys.argv[3], encoding="utf-8") as response_file:
    second_login_response = json.load(response_file)

expected_second_user_id = int(sys.argv[4])
expected_second_user_login_id = sys.argv[5]

if unauthorized_promote_response.get("error", {}).get("code") != "superadmin_bearer_token_required":
    raise SystemExit("unexpected error code for unauthorized promote response")

if promote_admin_response.get("user_id") != expected_second_user_id:
    raise SystemExit("promote admin response user_id mismatch")

if promote_admin_response.get("permission_level") != 1:
    raise SystemExit("expected promoted user permission_level to be 1")

if promote_admin_response.get("role_name") != "admin":
    raise SystemExit("expected promoted user role_name to be admin")

if second_login_response.get("user_id") != expected_second_user_id:
    raise SystemExit("second login response user_id mismatch")

if second_login_response.get("permission_level") != 1:
    raise SystemExit("expected second login response permission_level to be 1 after promotion")

if second_login_response.get("role_name") != "admin":
    raise SystemExit("expected second login response role_name to be admin after promotion")

if second_login_response.get("user_login_id") != expected_second_user_login_id:
    raise SystemExit("expected second login response user_login_id to be preserved")

if "is_admin" in promote_admin_response or "is_admin" in second_login_response:
    raise SystemExit("did not expect is_admin in auth responses")
PY
then
    append_log_line "${test_log_temp_file}" "admin promote validation failed"
    publish_failure_logs
    exit 1
fi

second_login_token="$(read_json_field "${second_login_response_file}" "token" "string")"
send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/user/${login_user_id}/user" \
    "${unauthorized_demote_response_file}" \
    "401" \
    "unauthorized demote by admin" \
    "${second_login_token}"
print_success_log "unauthorized demote by admin success"
assert_json_error_code \
    "${unauthorized_demote_response_file}" \
    "superadmin_bearer_token_required" \
    "unauthorized demote by admin"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/user/${second_user_id}/user" \
    "${demote_user_response_file}" \
    "200" \
    "demote user" \
    "${login_token}"
print_success_log "demote user success"

send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/auth/login" \
    "${third_login_response_file}" \
    "200" \
    "third login" \
    "" \
    "${second_login_request_body}"
print_success_log "third login success"

if ! python3 \
    - "${demote_user_response_file}" \
    "${third_login_response_file}" \
    "${second_user_id}" \
    "${second_user_login_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    demote_user_response = json.load(response_file)

with open(sys.argv[2], encoding="utf-8") as response_file:
    third_login_response = json.load(response_file)

expected_second_user_id = int(sys.argv[3])
expected_second_user_login_id = sys.argv[4]

if demote_user_response.get("user_id") != expected_second_user_id:
    raise SystemExit("demote user response user_id mismatch")

if demote_user_response.get("permission_level") != 0:
    raise SystemExit("expected demoted user permission_level to be 0")

if demote_user_response.get("role_name") != "user":
    raise SystemExit("expected demoted user role_name to be user")

if third_login_response.get("user_id") != expected_second_user_id:
    raise SystemExit("third login response user_id mismatch")

if third_login_response.get("permission_level") != 0:
    raise SystemExit("expected third login response permission_level to be 0 after demotion")

if third_login_response.get("role_name") != "user":
    raise SystemExit("expected third login response role_name to be user after demotion")

if third_login_response.get("user_login_id") != expected_second_user_login_id:
    raise SystemExit("expected third login response user_login_id to be preserved")

if "is_admin" in demote_user_response or "is_admin" in third_login_response:
    raise SystemExit("did not expect is_admin in demotion responses")
PY
then
    append_log_line "${test_log_temp_file}" "user demotion validation failed"
    publish_failure_logs
    exit 1
fi
print_success_log "user demotion success"

renew_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/auth/token/renew" \
        "${renew_response_file}" \
        "${login_token}"
)"
assert_status_code "${renew_status_code}" "200" "${renew_response_file}" "token renew"
print_success_log "token renew success"

logout_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/auth/logout" \
        "${logout_response_file}" \
        "${login_token}"
)"
assert_status_code "${logout_status_code}" "200" "${logout_response_file}" "logout"
print_success_log "logout success"

second_logout_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/auth/logout" \
        "${second_logout_response_file}" \
        "${login_token}"
)"
assert_status_code "${second_logout_status_code}" "401" "${second_logout_response_file}" "second logout"
print_success_log "token reuse failure success"
print_success_log "auth flow test passed: login_id=${user_login_id}, user_id=${login_user_id}"
