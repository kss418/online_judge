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

http_port="${HTTP_ERROR_CONTRACT_FLOW_TEST_HTTP_PORT:-18087}"
base_url="${HTTP_ERROR_CONTRACT_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${HTTP_ERROR_CONTRACT_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${HTTP_ERROR_CONTRACT_FLOW_TEST_LOGIN_ID:-$(make_test_login_id ec)}"
admin_user_login_id="${HTTP_ERROR_CONTRACT_FLOW_TEST_ADMIN_LOGIN_ID:-$(make_test_login_id ec a)}"
raw_password="${HTTP_ERROR_CONTRACT_FLOW_TEST_PASSWORD:-password123}"
test_db_name="http_error_contract_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_http_error_contract_flow.log"
server_log_name="test_http_error_contract_flow_server.log"

init_flow_test
register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file sign_up_response_file
register_temp_file create_problem_response_file
register_temp_file contract_response_file
register_temp_file create_submission_response_file

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
append_log_line "${test_log_temp_file}" "admin_user_login_id=${admin_user_login_id}"
append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"

apply_test_database_migrations
ensure_dedicated_http_server

run_error_contract_case(){
    local failure_context="$1"
    local request_method="$2"
    local request_url="$3"
    local auth_token="$4"
    local request_body="$5"
    local expected_status_code="$6"
    local expected_error_code="$7"
    local expected_error_message="$8"
    local expected_error_field="${9:-}"

    send_http_request_and_assert_status \
        "${request_method}" \
        "${request_url}" \
        "${contract_response_file}" \
        "${expected_status_code}" \
        "${failure_context}" \
        "${auth_token}" \
        "${request_body}"
    assert_json_error_code \
        "${contract_response_file}" \
        "${expected_error_code}" \
        "${failure_context}"
    assert_json_error_message \
        "${contract_response_file}" \
        "${expected_error_message}" \
        "${failure_context}"

    if [[ -n "${expected_error_field}" ]]; then
        assert_json_error_field \
            "${contract_response_file}" \
            "${expected_error_field}" \
            "${failure_context}"
    fi

    print_success_log "${failure_context} contract success"
}

run_error_contract_table(){
    local table_name="$1"
    local table_rows="$2"
    local case_name=""
    local request_method=""
    local request_url=""
    local auth_token=""
    local request_body=""
    local expected_status_code=""
    local expected_error_code=""
    local expected_error_message=""
    local expected_error_field=""

    while IFS='|' read -r \
        case_name \
        request_method \
        request_url \
        auth_token \
        request_body \
        expected_status_code \
        expected_error_code \
        expected_error_message \
        expected_error_field; do
        if [[ -z "${case_name}" ]]; then
            continue
        fi

        run_error_contract_case \
            "${table_name}: ${case_name}" \
            "${request_method}" \
            "${request_url}" \
            "${auth_token}" \
            "${request_body}" \
            "${expected_status_code}" \
            "${expected_error_code}" \
            "${expected_error_message}" \
            "${expected_error_field}"
    done <<< "${table_rows}"
}

create_testcase_fixture(){
    local testcase_request_body="$1"
    local failure_context="$2"

    send_http_request_and_assert_status \
        "POST" \
        "${base_url}/api/problem/${problem_id}/testcase" \
        "${contract_response_file}" \
        "201" \
        "${failure_context}" \
        "${admin_user_token}" \
        "${testcase_request_body}"
}

create_submission_fixture(){
    local submission_request_body="$1"
    local failure_context="$2"
    local create_submission_status_code=""

    create_submission_status_code="$(
        send_http_request \
            "POST" \
            "${base_url}/api/submission/${problem_id}" \
            "${create_submission_response_file}" \
            "${user_token}" \
            "${submission_request_body}"
    )"
    assert_status_code \
        "${create_submission_status_code}" \
        "201" \
        "${create_submission_response_file}" \
        "${failure_context}"

    python3 - "${create_submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

submission_id = response.get("submission_id")
status = response.get("status")
if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid submission_id in create submission response")
if status != "queued":
    raise SystemExit("expected created submission status to be queued")

print(submission_id)
PY
}

read -r user_id user_token < <(
    sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "http error contract flow"
)
print_success_log "contract user sign-up success"

read -r admin_user_id admin_user_token < <(
    sign_up_user "${admin_user_login_id}" "${raw_password}" "${sign_up_response_file}" "http error contract flow"
)
print_success_log "contract admin sign-up success"

promote_admin_user "${admin_user_id}" "http error contract flow" >/dev/null
print_success_log "contract admin promote success"

missing_user_id=$((admin_user_id + 999999))
missing_user_login_id="${HTTP_ERROR_CONTRACT_FLOW_TEST_MISSING_LOGIN_ID:-$(make_test_login_id ecm)}"
invalid_token="${HTTP_ERROR_CONTRACT_FLOW_TEST_INVALID_TOKEN:-definitely-invalid-token}"

problem_id="$(
    create_problem_via_api \
        "${admin_user_token}" \
        "${create_problem_response_file}" \
        "http error contract flow" \
        "HTTP Error Contract Flow"
)"
missing_problem_id=$((problem_id + 999999))
missing_sample_order=999
missing_testcase_order=999
missing_submission_id=999999
print_success_log "contract problem create success"

sample_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "sample_input": "10 20\n",
            "sample_output": "30\n",
        },
        separators=(",", ":")
    )
)
PY
)"

testcase_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "testcase_input": "1 2\n",
            "testcase_output": "3\n",
        },
        separators=(",", ":")
    )
)
PY
)"

second_testcase_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "testcase_input": "5 6\n",
            "testcase_output": "11\n",
        },
        separators=(",", ":")
    )
)
PY
)"

move_missing_source_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "source_testcase_order": 999,
            "target_testcase_order": 1,
        },
        separators=(",", ":")
    )
)
PY
)"

move_conflict_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "source_testcase_order": 1,
            "target_testcase_order": 3,
        },
        separators=(",", ":")
    )
)
PY
)"

invalid_submission_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "language": "ruby",
            "source_code": "print(1)\n",
        },
        separators=(",", ":")
    )
)
PY
)"

valid_submission_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "language": "cpp",
            "source_code": "#include <iostream>\nint main(){ return 0; }\n",
        },
        separators=(",", ":")
    )
)
PY
)"

invalid_login_request_body="$(
    python3 - "${user_login_id}" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "user_login_id": sys.argv[1],
            "raw_password": "wrongpass1",
        },
        separators=(",", ":")
    )
)
PY
)"

submission_ban_request_body="$(
    python3 <<'PY'
import json

print(json.dumps({"duration_minutes": 60}, separators=(",", ":")))
PY
)"

auth_contract_cases="$(cat <<EOF
invalid credentials login|POST|${base_url}/api/auth/login||${invalid_login_request_body}|401|invalid_credentials|invalid credentials|
invalid token renew|POST|${base_url}/api/auth/token/renew|${invalid_token}||401|invalid_or_expired_token|invalid, expired, or revoked token|
invalid token logout|POST|${base_url}/api/auth/logout|${invalid_token}||401|invalid_or_expired_token|invalid, expired, or revoked token|
EOF
)"
run_error_contract_table "auth contract" "${auth_contract_cases}"

user_contract_cases="$(cat <<EOF
missing public user summary by login id|GET|${base_url}/api/user/id/${missing_user_login_id}|||404|not_found|not found|
missing user statistics by id|GET|${base_url}/api/user/${missing_user_id}/statistics|||404|not_found|not found|
missing user submission ban get|GET|${base_url}/api/user/${missing_user_id}/submission-ban|${admin_user_token}||404|not_found|not found|
missing user submission ban create|POST|${base_url}/api/user/${missing_user_id}/submission-ban|${admin_user_token}|${submission_ban_request_body}|404|not_found|not found|
missing user submission ban delete|DELETE|${base_url}/api/user/${missing_user_id}/submission-ban|${admin_user_token}||404|not_found|not found|
EOF
)"
run_error_contract_table "user contract" "${user_contract_cases}"

sample_contract_cases="$(cat <<EOF
empty sample delete|DELETE|${base_url}/api/problem/${problem_id}/sample|${admin_user_token}||400|validation_error|missing sample to delete|
missing problem sample create|POST|${base_url}/api/problem/${missing_problem_id}/sample|${admin_user_token}||404|not_found|not found|
missing sample update|PUT|${base_url}/api/problem/${problem_id}/sample/${missing_sample_order}|${admin_user_token}|${sample_request_body}|404|not_found|not found|
EOF
)"
run_error_contract_table "sample contract" "${sample_contract_cases}"

create_testcase_fixture "${testcase_request_body}" "contract testcase fixture create #1"
create_testcase_fixture "${second_testcase_request_body}" "contract testcase fixture create #2"
print_success_log "contract testcase fixtures ready"

testcase_contract_cases="$(cat <<EOF
missing problem testcase create|POST|${base_url}/api/problem/${missing_problem_id}/testcase|${admin_user_token}|${testcase_request_body}|404|not_found|not found|
missing testcase update|PUT|${base_url}/api/problem/${problem_id}/testcase/${missing_testcase_order}|${admin_user_token}|${testcase_request_body}|404|not_found|not found|
missing testcase move source|POST|${base_url}/api/problem/${problem_id}/testcase/move|${admin_user_token}|${move_missing_source_request_body}|404|not_found|not found|
invalid testcase move target|POST|${base_url}/api/problem/${problem_id}/testcase/move|${admin_user_token}|${move_conflict_request_body}|409|conflict|conflict|
EOF
)"
run_error_contract_table "testcase contract" "${testcase_contract_cases}"

submission_id="$(
    create_submission_fixture \
        "${valid_submission_request_body}" \
        "contract submission fixture create"
)"
print_success_log "contract submission fixture ready"

submission_contract_cases="$(cat <<EOF
invalid submission language|POST|${base_url}/api/submission/${problem_id}|${user_token}|${invalid_submission_request_body}|400|invalid_field|unsupported language: ruby|language
missing problem submission create|POST|${base_url}/api/submission/${missing_problem_id}|${user_token}|${valid_submission_request_body}|404|not_found|not found|
queued submission rejudge|POST|${base_url}/api/submission/${submission_id}/rejudge|${admin_user_token}||409|conflict|conflict|
EOF
)"
run_error_contract_table "submission contract" "${submission_contract_cases}"

run_error_contract_case \
    "submission contract: missing submission detail" \
    "GET" \
    "${base_url}/api/submission/${missing_submission_id}" \
    "" \
    "" \
    "404" \
    "not_found" \
    "not found"

append_log_line "${test_log_temp_file}" "http error contract flow test passed"
print_success_log \
    "http error contract flow test passed: user_id=${user_id}, admin_user_id=${admin_user_id}, problem_id=${problem_id}, submission_id=${submission_id}"
