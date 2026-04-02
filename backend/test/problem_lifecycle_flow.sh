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

http_port="${PROBLEM_LIFECYCLE_FLOW_TEST_HTTP_PORT:-18087}"
base_url="${PROBLEM_LIFECYCLE_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${PROBLEM_LIFECYCLE_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${PROBLEM_LIFECYCLE_FLOW_TEST_LOGIN_ID:-$(make_test_login_id lc)}"
raw_password="${PROBLEM_LIFECYCLE_FLOW_TEST_PASSWORD:-password123}"
test_db_name="problem_lifecycle_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_problem_lifecycle_flow.log"
server_log_name="test_problem_lifecycle_flow_server.log"

init_flow_test
register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file sign_up_response_file
register_temp_file create_problem_response_file
register_temp_file update_problem_response_file
register_temp_file get_problem_response_file
register_temp_file delete_problem_response_file
register_temp_file missing_problem_response_file

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
    sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "problem lifecycle flow"
)
promote_admin_user "${sign_up_user_id}" "problem lifecycle flow" >/dev/null

problem_id="$(
    create_problem_via_api \
        "${sign_up_token}" \
        "${create_problem_response_file}" \
        "problem lifecycle flow" \
        "Problem Lifecycle Flow"
)"

update_problem_request_body="$(
    make_create_problem_request_body "Problem Lifecycle Flow Updated"
)"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/problem/${problem_id}/title" \
    "${update_problem_response_file}" \
    "200" \
    "update problem title" \
    "${sign_up_token}" \
    "${update_problem_request_body}"
assert_json_message \
    "${update_problem_response_file}" \
    "problem updated" \
    "update problem title"

print_success_log "problem title update success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${get_problem_response_file}" \
    "200" \
    "get updated problem"
assert_json_field_equals \
    "${get_problem_response_file}" \
    "problem_id" \
    "${problem_id}" \
    "get updated problem" \
    "int"
assert_json_field_equals \
    "${get_problem_response_file}" \
    "title" \
    "Problem Lifecycle Flow Updated" \
    "get updated problem"
assert_json_field_equals \
    "${get_problem_response_file}" \
    "version" \
    "2" \
    "get updated problem" \
    "int"
assert_json_field_equals \
    "${get_problem_response_file}" \
    "limits.memory_limit_mb" \
    "256" \
    "get updated problem" \
    "int"
assert_json_field_equals \
    "${get_problem_response_file}" \
    "limits.time_limit_ms" \
    "1000" \
    "get updated problem" \
    "int"
assert_json_field_equals \
    "${get_problem_response_file}" \
    "statement" \
    "" \
    "get updated problem" \
    "null"
assert_json_field_equals \
    "${get_problem_response_file}" \
    "sample_count" \
    "0" \
    "get updated problem" \
    "int"
assert_json_field_equals \
    "${get_problem_response_file}" \
    "statistics.submission_count" \
    "0" \
    "get updated problem" \
    "int"
assert_json_field_equals \
    "${get_problem_response_file}" \
    "statistics.accepted_count" \
    "0" \
    "get updated problem" \
    "int"

print_success_log "updated problem detail validated"

send_http_request_and_assert_status \
    "DELETE" \
    "${base_url}/api/problem/${problem_id}" \
    "${delete_problem_response_file}" \
    "200" \
    "delete problem" \
    "${sign_up_token}"
assert_json_message \
    "${delete_problem_response_file}" \
    "problem deleted" \
    "delete problem"

print_success_log "problem delete success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${missing_problem_response_file}" \
    "404" \
    "get deleted problem"
assert_json_error_code \
    "${missing_problem_response_file}" \
    "not_found" \
    "get deleted problem"
assert_json_error_message \
    "${missing_problem_response_file}" \
    "not found" \
    "get deleted problem"

append_log_line "${test_log_temp_file}" "problem lifecycle flow test passed"
print_success_log \
    "problem lifecycle flow test passed: problem_id=${problem_id}, admin_user_id=${sign_up_user_id}"
