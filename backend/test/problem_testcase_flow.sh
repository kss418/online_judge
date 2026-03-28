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

http_port="${PROBLEM_TESTCASE_FLOW_TEST_HTTP_PORT:-18083}"
base_url="${PROBLEM_TESTCASE_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${PROBLEM_TESTCASE_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${PROBLEM_TESTCASE_FLOW_TEST_LOGIN_ID:-problem_testcase_flow_test_$(date +%s)_$$}"
raw_password="${PROBLEM_TESTCASE_FLOW_TEST_PASSWORD:-password123}"
test_db_name="problem_testcase_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_problem_testcase_flow.log"
server_log_name="test_problem_testcase_flow_server.log"
test_log_temp_file="$(mktemp)"
server_log_temp_file="$(mktemp)"
sign_up_response_file="$(mktemp)"
create_problem_response_file="$(mktemp)"
create_testcase_response_file="$(mktemp)"
create_empty_testcase_response_file="$(mktemp)"
list_testcases_response_file="$(mktemp)"
update_testcase_response_file="$(mktemp)"
updated_testcases_response_file="$(mktemp)"
move_testcase_response_file="$(mktemp)"
move_testcases_response_file="$(mktemp)"
delete_testcase_response_file="$(mktemp)"
delete_all_testcases_response_file="$(mktemp)"
remaining_testcases_response_file="$(mktemp)"
cleared_testcases_response_file="$(mktemp)"
delete_all_empty_testcases_response_file="$(mktemp)"
upload_testcase_zip_response_file="$(mktemp)"
uploaded_testcases_response_file="$(mktemp)"
uploaded_problem_response_file="$(mktemp)"
replace_testcase_zip_response_file="$(mktemp)"
replaced_testcases_response_file="$(mktemp)"
replaced_problem_response_file="$(mktemp)"
invalid_unpaired_zip_response_file="$(mktemp)"
invalid_duplicate_zip_response_file="$(mktemp)"
invalid_malformed_zip_response_file="$(mktemp)"
get_problem_response_file="$(mktemp)"
updated_problem_response_file="$(mktemp)"
deleted_problem_response_file="$(mktemp)"
cleared_problem_response_file="$(mktemp)"
final_problem_response_file="$(mktemp)"
valid_testcase_zip_path="$(mktemp)"
replace_testcase_zip_path="$(mktemp)"
invalid_unpaired_zip_path="$(mktemp)"
invalid_duplicate_zip_path="$(mktemp)"
invalid_malformed_zip_path="$(mktemp)"

cleanup(){
    cleanup_http_server
    drop_test_database

    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${sign_up_response_file}" \
        "${create_problem_response_file}" \
        "${create_testcase_response_file}" \
        "${create_empty_testcase_response_file}" \
        "${list_testcases_response_file}" \
        "${update_testcase_response_file}" \
        "${updated_testcases_response_file}" \
        "${move_testcase_response_file}" \
        "${move_testcases_response_file}" \
        "${delete_testcase_response_file}" \
        "${delete_all_testcases_response_file}" \
        "${remaining_testcases_response_file}" \
        "${cleared_testcases_response_file}" \
        "${delete_all_empty_testcases_response_file}" \
        "${upload_testcase_zip_response_file}" \
        "${uploaded_testcases_response_file}" \
        "${uploaded_problem_response_file}" \
        "${replace_testcase_zip_response_file}" \
        "${replaced_testcases_response_file}" \
        "${replaced_problem_response_file}" \
        "${invalid_unpaired_zip_response_file}" \
        "${invalid_duplicate_zip_response_file}" \
        "${invalid_malformed_zip_response_file}" \
        "${get_problem_response_file}" \
        "${updated_problem_response_file}" \
        "${deleted_problem_response_file}" \
        "${cleared_problem_response_file}" \
        "${final_problem_response_file}" \
        "${valid_testcase_zip_path}" \
        "${replace_testcase_zip_path}" \
        "${invalid_unpaired_zip_path}" \
        "${invalid_duplicate_zip_path}" \
        "${invalid_malformed_zip_path}"

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

send_zip_request_and_assert_status(){
    local request_url="$1"
    local zip_file_path="$2"
    local response_file_path="$3"
    local expected_status_code="$4"
    local failure_context="$5"
    local auth_token="$6"
    local actual_status_code=""

    actual_status_code="$(
        curl \
            --silent \
            --show-error \
            --output "${response_file_path}" \
            --write-out "%{http_code}" \
            --request POST \
            -H "Authorization: Bearer ${auth_token}" \
            -H "Content-Type: application/zip" \
            --data-binary "@${zip_file_path}" \
            "${request_url}"
    )"

    assert_status_code \
        "${actual_status_code}" \
        "${expected_status_code}" \
        "${response_file_path}" \
        "${failure_context}"
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
    sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "problem testcase flow"
)
promote_admin_user "${sign_up_user_id}" "problem testcase flow" >/dev/null
problem_id="$(
    create_problem_via_api \
        "${sign_up_token}" \
        "${create_problem_response_file}" \
        "problem testcase flow" \
        "Problem Testcase Flow"
)"

testcase_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "testcase_input": "1 2\n",
            "testcase_output": "3\n",
        }
    )
)
PY
)"

send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/problem/${problem_id}/testcase" \
    "${create_testcase_response_file}" \
    "201" \
    "first testcase create" \
    "${sign_up_token}" \
    "${testcase_request_body}"

python3 - "${create_testcase_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

testcase_id = response.get("testcase_id")
testcase_order = response.get("testcase_order")
if not isinstance(testcase_id, int) or testcase_id <= 0:
    raise SystemExit("invalid testcase_id after first testcase create")
if testcase_order != 1:
    raise SystemExit("unexpected testcase_order after first testcase create")
PY

print_success_log "problem testcase first create success"

empty_testcase_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "testcase_input": "",
            "testcase_output": "",
        }
    )
)
PY
)"

send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/problem/${problem_id}/testcase" \
    "${create_empty_testcase_response_file}" \
    "201" \
    "empty testcase create" \
    "${sign_up_token}" \
    "${empty_testcase_request_body}"

python3 - "${create_empty_testcase_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

testcase_id = response.get("testcase_id")
testcase_order = response.get("testcase_order")
if not isinstance(testcase_id, int) or testcase_id <= 0:
    raise SystemExit("invalid testcase_id after empty testcase create")
if testcase_order != 2:
    raise SystemExit("unexpected testcase_order after empty testcase create")
PY

print_success_log "problem testcase empty create success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/testcase" \
    "${list_testcases_response_file}" \
    "200" \
    "list testcases" \
    "${sign_up_token}"

python3 - "${list_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 2:
    raise SystemExit("unexpected testcase_count after list")

testcases = response.get("testcases")
if not isinstance(testcases, list) or len(testcases) != 2:
    raise SystemExit("unexpected testcase list size")

first_testcase = testcases[0]
second_testcase = testcases[1]

if first_testcase.get("testcase_order") != 1:
    raise SystemExit("unexpected first testcase_order after list")
if first_testcase.get("testcase_input") != "1 2\n":
    raise SystemExit("unexpected first testcase_input after list")
if first_testcase.get("testcase_output") != "3\n":
    raise SystemExit("unexpected first testcase_output after list")

if second_testcase.get("testcase_order") != 2:
    raise SystemExit("unexpected second testcase_order after list")
if second_testcase.get("testcase_input") != "":
    raise SystemExit("unexpected second testcase_input after list")
if second_testcase.get("testcase_output") != "":
    raise SystemExit("unexpected second testcase_output after list")
PY

print_success_log "problem testcase list success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${get_problem_response_file}" \
    "200" \
    "get problem after testcase create"

python3 - "${get_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after testcase create")
if response.get("title") != "Problem Testcase Flow":
    raise SystemExit("title mismatch after testcase create")
if response.get("version") != 3:
    raise SystemExit("version mismatch after testcase create")
if response.get("limits") != {"memory_limit_mb": 256, "time_limit_ms": 1000}:
    raise SystemExit("limits changed unexpectedly after testcase create")
if response.get("statement", "missing") is not None:
    raise SystemExit("expected null statement after testcase create")
if response.get("sample_count") != 0:
    raise SystemExit("sample_count changed unexpectedly after testcase create")
if response.get("samples") != []:
    raise SystemExit("samples changed unexpectedly after testcase create")
statistics = response.get("statistics")
if statistics != {"submission_count": 0, "accepted_count": 0}:
    raise SystemExit("statistics changed unexpectedly after testcase create")
if "testcase_count" in response:
    raise SystemExit("unexpected testcase_count field")
PY

update_testcase_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "testcase_input": "10 20\n",
            "testcase_output": "30\n",
        }
    )
)
PY
)"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/problem/${problem_id}/testcase/1" \
    "${update_testcase_response_file}" \
    "200" \
    "update testcase" \
    "${sign_up_token}" \
    "${update_testcase_request_body}"

python3 - "${update_testcase_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_order") != 1:
    raise SystemExit("unexpected testcase_order after update")
if response.get("testcase_input") != "10 20\n":
    raise SystemExit("unexpected testcase_input after update")
if response.get("testcase_output") != "30\n":
    raise SystemExit("unexpected testcase_output after update")
testcase_id = response.get("testcase_id")
if not isinstance(testcase_id, int) or testcase_id <= 0:
    raise SystemExit("invalid testcase_id after update")
PY

print_success_log "problem testcase update success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/testcase" \
    "${updated_testcases_response_file}" \
    "200" \
    "list updated testcases" \
    "${sign_up_token}"

python3 - "${updated_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 2:
    raise SystemExit("unexpected testcase_count after update")

testcases = response.get("testcases")
if not isinstance(testcases, list) or len(testcases) != 2:
    raise SystemExit("unexpected testcase list size after update")

first_testcase = testcases[0]
if first_testcase.get("testcase_order") != 1:
    raise SystemExit("unexpected first testcase_order after update")
if first_testcase.get("testcase_input") != "10 20\n":
    raise SystemExit("unexpected first testcase_input after update")
if first_testcase.get("testcase_output") != "30\n":
    raise SystemExit("unexpected first testcase_output after update")
PY

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${updated_problem_response_file}" \
    "200" \
    "get problem after testcase update"

python3 - "${updated_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after testcase update")
if response.get("version") != 4:
    raise SystemExit("version mismatch after testcase update")
PY

send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/problem/${problem_id}/testcase/move" \
    "${move_testcase_response_file}" \
    "200" \
    "move testcase" \
    "${sign_up_token}" \
    "$(
        python3 <<'PY'
import json

print(
    json.dumps(
        {
            "source_testcase_order": 2,
            "target_testcase_order": 1,
        }
    )
)
PY
    )"
assert_json_message \
    "${move_testcase_response_file}" \
    "problem testcase moved" \
    "move testcase"

print_success_log "problem testcase move success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/testcase" \
    "${move_testcases_response_file}" \
    "200" \
    "list moved testcases" \
    "${sign_up_token}"

python3 - "${move_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 2:
    raise SystemExit("unexpected testcase_count after move")

testcases = response.get("testcases")
if not isinstance(testcases, list) or len(testcases) != 2:
    raise SystemExit("unexpected testcase list size after move")

first_testcase = testcases[0]
second_testcase = testcases[1]

if first_testcase.get("testcase_order") != 1:
    raise SystemExit("unexpected first testcase_order after move")
if first_testcase.get("testcase_input") != "":
    raise SystemExit("unexpected first testcase_input after move")
if first_testcase.get("testcase_output") != "":
    raise SystemExit("unexpected first testcase_output after move")

if second_testcase.get("testcase_order") != 2:
    raise SystemExit("unexpected second testcase_order after move")
if second_testcase.get("testcase_input") != "10 20\n":
    raise SystemExit("unexpected second testcase_input after move")
if second_testcase.get("testcase_output") != "30\n":
    raise SystemExit("unexpected second testcase_output after move")
PY

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${deleted_problem_response_file}" \
    "200" \
    "get problem after testcase move"

python3 - "${deleted_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after testcase move")
if response.get("version") != 5:
    raise SystemExit("version mismatch after testcase move")
PY

send_http_request_and_assert_status \
    "DELETE" \
    "${base_url}/api/problem/${problem_id}/testcase/1" \
    "${delete_testcase_response_file}" \
    "200" \
    "delete testcase" \
    "${sign_up_token}"
assert_json_message \
    "${delete_testcase_response_file}" \
    "problem testcase deleted" \
    "delete testcase"

print_success_log "problem testcase delete success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/testcase" \
    "${remaining_testcases_response_file}" \
    "200" \
    "list remaining testcases" \
    "${sign_up_token}"

python3 - "${remaining_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 1:
    raise SystemExit("unexpected testcase_count after delete")

testcases = response.get("testcases")
if not isinstance(testcases, list) or len(testcases) != 1:
    raise SystemExit("unexpected testcase list size after delete")

first_testcase = testcases[0]
if first_testcase.get("testcase_order") != 1:
    raise SystemExit("unexpected remaining testcase_order after delete")
if first_testcase.get("testcase_input") != "10 20\n":
    raise SystemExit("unexpected remaining testcase_input after delete")
if first_testcase.get("testcase_output") != "30\n":
    raise SystemExit("unexpected remaining testcase_output after delete")
PY

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${deleted_problem_response_file}" \
    "200" \
    "get problem after testcase delete"

python3 - "${deleted_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after testcase delete")
if response.get("version") != 6:
    raise SystemExit("version mismatch after testcase delete")
PY

send_http_request_and_assert_status \
    "DELETE" \
    "${base_url}/api/problem/${problem_id}/testcase/all" \
    "${delete_all_testcases_response_file}" \
    "200" \
    "delete all testcases" \
    "${sign_up_token}"
assert_json_message \
    "${delete_all_testcases_response_file}" \
    "problem testcases deleted" \
    "delete all testcases"

print_success_log "problem testcase delete all success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/testcase" \
    "${cleared_testcases_response_file}" \
    "200" \
    "list cleared testcases" \
    "${sign_up_token}"

python3 - "${cleared_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 0:
    raise SystemExit("unexpected testcase_count after delete all")
if response.get("testcases") != []:
    raise SystemExit("unexpected testcase list after delete all")
PY

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${cleared_problem_response_file}" \
    "200" \
    "get problem after delete all testcases"

python3 - "${cleared_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after delete all testcases")
if response.get("version") != 7:
    raise SystemExit("version mismatch after delete all testcases")
PY

send_http_request_and_assert_status \
    "DELETE" \
    "${base_url}/api/problem/${problem_id}/testcase/all" \
    "${delete_all_empty_testcases_response_file}" \
    "200" \
    "delete all empty testcases" \
    "${sign_up_token}"
assert_json_message \
    "${delete_all_empty_testcases_response_file}" \
    "problem testcases deleted" \
    "delete all empty testcases"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${final_problem_response_file}" \
    "200" \
    "get problem after empty delete all testcases"

python3 - "${final_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after empty delete all testcases")
if response.get("version") != 7:
    raise SystemExit("version mismatch after empty delete all testcases")
PY

python3 - "${valid_testcase_zip_path}" <<'PY'
import sys
import zipfile

with zipfile.ZipFile(sys.argv[1], "w") as testcase_zip:
    testcase_zip.writestr("001.in", "4 5\n")
    testcase_zip.writestr("001.out", "9\n")
    testcase_zip.writestr("002.in", "")
    testcase_zip.writestr("002.out", "")
PY

send_zip_request_and_assert_status \
    "${base_url}/api/problem/${problem_id}/testcase/zip" \
    "${valid_testcase_zip_path}" \
    "${upload_testcase_zip_response_file}" \
    "200" \
    "upload testcase zip" \
    "${sign_up_token}"
assert_json_message \
    "${upload_testcase_zip_response_file}" \
    "problem testcases uploaded" \
    "upload testcase zip"
assert_json_field_equals \
    "${upload_testcase_zip_response_file}" \
    "testcase_count" \
    "2" \
    "upload testcase zip count" \
    "int"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/testcase" \
    "${uploaded_testcases_response_file}" \
    "200" \
    "list uploaded testcases" \
    "${sign_up_token}"

python3 - "${uploaded_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 2:
    raise SystemExit("unexpected testcase_count after testcase zip upload")

testcases = response.get("testcases")
if not isinstance(testcases, list) or len(testcases) != 2:
    raise SystemExit("unexpected testcase list size after testcase zip upload")

first_testcase = testcases[0]
second_testcase = testcases[1]

if first_testcase.get("testcase_order") != 1:
    raise SystemExit("unexpected first testcase_order after testcase zip upload")
if first_testcase.get("testcase_input") != "4 5\n":
    raise SystemExit("unexpected first testcase_input after testcase zip upload")
if first_testcase.get("testcase_output") != "9\n":
    raise SystemExit("unexpected first testcase_output after testcase zip upload")

if second_testcase.get("testcase_order") != 2:
    raise SystemExit("unexpected second testcase_order after testcase zip upload")
if second_testcase.get("testcase_input") != "":
    raise SystemExit("unexpected second testcase_input after testcase zip upload")
if second_testcase.get("testcase_output") != "":
    raise SystemExit("unexpected second testcase_output after testcase zip upload")
PY

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${uploaded_problem_response_file}" \
    "200" \
    "get problem after testcase zip upload"

python3 - "${uploaded_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after testcase zip upload")
if response.get("version") != 8:
    raise SystemExit("version mismatch after testcase zip upload")
PY

python3 - "${replace_testcase_zip_path}" <<'PY'
import sys
import zipfile

with zipfile.ZipFile(sys.argv[1], "w") as testcase_zip:
    testcase_zip.writestr("001.in", "7 8\n")
    testcase_zip.writestr("001.out", "15\n")
PY

send_zip_request_and_assert_status \
    "${base_url}/api/problem/${problem_id}/testcase/zip" \
    "${replace_testcase_zip_path}" \
    "${replace_testcase_zip_response_file}" \
    "200" \
    "replace testcase zip" \
    "${sign_up_token}"
assert_json_message \
    "${replace_testcase_zip_response_file}" \
    "problem testcases uploaded" \
    "replace testcase zip"
assert_json_field_equals \
    "${replace_testcase_zip_response_file}" \
    "testcase_count" \
    "1" \
    "replace testcase zip count" \
    "int"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/testcase" \
    "${replaced_testcases_response_file}" \
    "200" \
    "list replaced testcases" \
    "${sign_up_token}"

python3 - "${replaced_testcases_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("testcase_count") != 1:
    raise SystemExit("unexpected testcase_count after testcase zip replace")

testcases = response.get("testcases")
if not isinstance(testcases, list) or len(testcases) != 1:
    raise SystemExit("unexpected testcase list size after testcase zip replace")

first_testcase = testcases[0]
if first_testcase.get("testcase_order") != 1:
    raise SystemExit("unexpected testcase_order after testcase zip replace")
if first_testcase.get("testcase_input") != "7 8\n":
    raise SystemExit("unexpected testcase_input after testcase zip replace")
if first_testcase.get("testcase_output") != "15\n":
    raise SystemExit("unexpected testcase_output after testcase zip replace")
PY

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${replaced_problem_response_file}" \
    "200" \
    "get problem after testcase zip replace"

python3 - "${replaced_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after testcase zip replace")
if response.get("version") != 9:
    raise SystemExit("version mismatch after testcase zip replace")
PY

python3 - "${invalid_unpaired_zip_path}" <<'PY'
import sys
import zipfile

with zipfile.ZipFile(sys.argv[1], "w") as testcase_zip:
    testcase_zip.writestr("001.in", "1\n")
PY

send_zip_request_and_assert_status \
    "${base_url}/api/problem/${problem_id}/testcase/zip" \
    "${invalid_unpaired_zip_path}" \
    "${invalid_unpaired_zip_response_file}" \
    "400" \
    "reject unpaired testcase zip" \
    "${sign_up_token}"
assert_json_field_equals \
    "${invalid_unpaired_zip_response_file}" \
    "error.code" \
    "invalid_testcase_zip" \
    "reject unpaired testcase zip code"

python3 - "${invalid_duplicate_zip_path}" <<'PY'
import sys
import warnings
import zipfile

warnings.simplefilter("ignore", UserWarning)
with zipfile.ZipFile(sys.argv[1], "w") as testcase_zip:
    testcase_zip.writestr("001.in", "1\n")
    testcase_zip.writestr("001.in", "2\n")
    testcase_zip.writestr("001.out", "3\n")
PY

send_zip_request_and_assert_status \
    "${base_url}/api/problem/${problem_id}/testcase/zip" \
    "${invalid_duplicate_zip_path}" \
    "${invalid_duplicate_zip_response_file}" \
    "400" \
    "reject duplicate testcase zip" \
    "${sign_up_token}"
assert_json_field_equals \
    "${invalid_duplicate_zip_response_file}" \
    "error.code" \
    "invalid_testcase_zip" \
    "reject duplicate testcase zip code"

python3 - "${invalid_malformed_zip_path}" <<'PY'
import sys

with open(sys.argv[1], "wb") as invalid_zip_file:
    invalid_zip_file.write(b"not-a-zip\n" * 200000)
PY

send_zip_request_and_assert_status \
    "${base_url}/api/problem/${problem_id}/testcase/zip" \
    "${invalid_malformed_zip_path}" \
    "${invalid_malformed_zip_response_file}" \
    "400" \
    "reject malformed testcase zip" \
    "${sign_up_token}"
assert_json_field_equals \
    "${invalid_malformed_zip_response_file}" \
    "error.code" \
    "invalid_testcase_zip" \
    "reject malformed testcase zip code"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${final_problem_response_file}" \
    "200" \
    "get problem after invalid testcase zip"

python3 - "${final_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch after invalid testcase zip")
if response.get("version") != 9:
    raise SystemExit("version mismatch after invalid testcase zip")
PY

append_log_line "${test_log_temp_file}" "problem testcase flow test passed"
print_success_log "problem testcase flow test passed: problem_id=${problem_id}, admin_user_id=${sign_up_user_id}"
