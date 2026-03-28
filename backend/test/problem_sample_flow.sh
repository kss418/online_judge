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

http_port="${PROBLEM_SAMPLE_FLOW_TEST_HTTP_PORT:-18084}"
base_url="${PROBLEM_SAMPLE_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${PROBLEM_SAMPLE_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${PROBLEM_SAMPLE_FLOW_TEST_LOGIN_ID:-$(make_test_login_id ps)}"
raw_password="${PROBLEM_SAMPLE_FLOW_TEST_PASSWORD:-password123}"
test_db_name="problem_sample_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_problem_sample_flow.log"
server_log_name="test_problem_sample_flow_server.log"
test_log_temp_file="$(mktemp)"
server_log_temp_file="$(mktemp)"
sign_up_response_file="$(mktemp)"
create_problem_response_file="$(mktemp)"
create_sample_response_file="$(mktemp)"
create_empty_sample_response_file="$(mktemp)"
list_samples_response_file="$(mktemp)"
get_problem_response_file="$(mktemp)"
update_sample_response_file="$(mktemp)"
updated_samples_response_file="$(mktemp)"
updated_problem_response_file="$(mktemp)"
delete_sample_response_file="$(mktemp)"
remaining_samples_response_file="$(mktemp)"
deleted_problem_response_file="$(mktemp)"

cleanup(){
    cleanup_http_server
    drop_test_database

    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${sign_up_response_file}" \
        "${create_problem_response_file}" \
        "${create_sample_response_file}" \
        "${create_empty_sample_response_file}" \
        "${list_samples_response_file}" \
        "${get_problem_response_file}" \
        "${update_sample_response_file}" \
        "${updated_samples_response_file}" \
        "${updated_problem_response_file}" \
        "${delete_sample_response_file}" \
        "${remaining_samples_response_file}" \
        "${deleted_problem_response_file}"
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
append_log_line "${test_log_temp_file}" "user_login_id=${user_login_id}"
append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"

apply_test_database_migrations
ensure_dedicated_http_server

read -r sign_up_user_id sign_up_token < <(
    sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "problem sample flow"
)
promote_admin_user "${sign_up_user_id}" "problem sample flow" >/dev/null
problem_id="$(
    create_problem_via_api \
        "${sign_up_token}" \
        "${create_problem_response_file}" \
        "problem sample flow" \
        "Problem Sample Flow"
)"

send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/problem/${problem_id}/sample" \
    "${create_sample_response_file}" \
    "201" \
    "first sample create" \
    "${sign_up_token}"

python3 - "${create_sample_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

sample_id = response.get("sample_id")
sample_order = response.get("sample_order")
if not isinstance(sample_id, int) or sample_id <= 0:
    raise SystemExit("invalid sample_id after first sample create")
if sample_order != 1:
    raise SystemExit("unexpected sample_order after first sample create")
PY

print_success_log "problem sample first create success"

send_http_request_and_assert_status \
    "POST" \
    "${base_url}/api/problem/${problem_id}/sample" \
    "${create_empty_sample_response_file}" \
    "201" \
    "empty sample create" \
    "${sign_up_token}"

python3 - "${create_empty_sample_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

sample_id = response.get("sample_id")
sample_order = response.get("sample_order")
if not isinstance(sample_id, int) or sample_id <= 0:
    raise SystemExit("invalid sample_id after empty sample create")
if sample_order != 2:
    raise SystemExit("unexpected sample_order after empty sample create")
PY

print_success_log "problem sample empty create success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/sample" \
    "${list_samples_response_file}" \
    "200" \
    "list samples"

python3 - "${list_samples_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("sample_count") != 2:
    raise SystemExit("unexpected sample_count after list")

samples = response.get("samples")
if not isinstance(samples, list) or len(samples) != 2:
    raise SystemExit("unexpected sample list size")

first_sample = samples[0]
second_sample = samples[1]

if not isinstance(first_sample.get("sample_id"), int) or first_sample["sample_id"] <= 0:
    raise SystemExit("invalid first sample_id after list")
if first_sample.get("sample_order") != 1:
    raise SystemExit("unexpected first sample_order after list")
if first_sample.get("sample_input") != "":
    raise SystemExit("unexpected first sample_input after list")
if first_sample.get("sample_output") != "":
    raise SystemExit("unexpected first sample_output after list")

if not isinstance(second_sample.get("sample_id"), int) or second_sample["sample_id"] <= 0:
    raise SystemExit("invalid second sample_id after list")
if second_sample.get("sample_order") != 2:
    raise SystemExit("unexpected second sample_order after list")
if second_sample.get("sample_input") != "":
    raise SystemExit("unexpected second sample_input after list")
if second_sample.get("sample_output") != "":
    raise SystemExit("unexpected second sample_output after list")
PY

print_success_log "problem sample list success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${get_problem_response_file}" \
    "200" \
    "get problem after sample create"

if ! python3 - "${get_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
expected_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch after sample create")
if response.get("title") != "Problem Sample Flow":
    raise SystemExit("title mismatch after sample create")
if response.get("version") != 1:
    raise SystemExit("version mismatch after sample create")
if response.get("limits") != {"memory_limit_mb": 256, "time_limit_ms": 1000}:
    raise SystemExit("limits changed unexpectedly after sample create")
if response.get("statement") is not None:
    raise SystemExit("statement changed unexpectedly after sample create")
if response.get("sample_count") != 2:
    raise SystemExit("sample_count mismatch after sample create")

expected_samples = [
    {
        "sample_order": 1,
        "sample_input": "",
        "sample_output": "",
    },
    {
        "sample_order": 2,
        "sample_input": "",
        "sample_output": "",
    },
]
if response.get("samples") != expected_samples:
    raise SystemExit("public samples mismatch after sample create")

if response.get("statistics") != {"submission_count": 0, "accepted_count": 0}:
    raise SystemExit("statistics changed unexpectedly after sample create")
PY
then
    append_log_line "${test_log_temp_file}" "public sample verification failed"
    publish_failure_logs
    exit 1
fi

update_sample_request_body="$(
    python3 <<'PY'
import json

print(
    json.dumps(
        {
            "sample_input": "10 20\n",
            "sample_output": "30\n",
        }
    )
)
PY
)"

send_http_request_and_assert_status \
    "PUT" \
    "${base_url}/api/problem/${problem_id}/sample/1" \
    "${update_sample_response_file}" \
    "200" \
    "update sample" \
    "${sign_up_token}" \
    "${update_sample_request_body}"

python3 - "${update_sample_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

sample_id = response.get("sample_id")
if not isinstance(sample_id, int) or sample_id <= 0:
    raise SystemExit("invalid sample_id after update")
if response.get("sample_order") != 1:
    raise SystemExit("unexpected sample_order after update")
if response.get("sample_input") != "10 20\n":
    raise SystemExit("unexpected sample_input after update")
if response.get("sample_output") != "30\n":
    raise SystemExit("unexpected sample_output after update")
PY

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/sample" \
    "${updated_samples_response_file}" \
    "200" \
    "list samples after update"

python3 - "${updated_samples_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

expected_samples = [
    {
        "sample_order": 1,
        "sample_input": "10 20\n",
        "sample_output": "30\n",
    },
    {
        "sample_order": 2,
        "sample_input": "",
        "sample_output": "",
    },
]

samples = response.get("samples")
if response.get("sample_count") != 2 or not isinstance(samples, list) or len(samples) != 2:
    raise SystemExit("unexpected sample list after update")

normalized_samples = [
    {
        "sample_order": sample.get("sample_order"),
        "sample_input": sample.get("sample_input"),
        "sample_output": sample.get("sample_output"),
    }
    for sample in samples
]
if normalized_samples != expected_samples:
    raise SystemExit("updated sample list mismatch")
PY

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${updated_problem_response_file}" \
    "200" \
    "get problem after sample update"

if ! python3 - "${updated_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
expected_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch after sample update")
if response.get("version") != 2:
    raise SystemExit("version mismatch after sample update")
if response.get("sample_count") != 2:
    raise SystemExit("sample_count mismatch after sample update")

expected_samples = [
    {
        "sample_order": 1,
        "sample_input": "10 20\n",
        "sample_output": "30\n",
    },
    {
        "sample_order": 2,
        "sample_input": "",
        "sample_output": "",
    },
]
if response.get("samples") != expected_samples:
    raise SystemExit("public samples mismatch after sample update")
PY
then
    append_log_line "${test_log_temp_file}" "updated public sample verification failed"
    publish_failure_logs
    exit 1
fi

send_http_request_and_assert_status \
    "DELETE" \
    "${base_url}/api/problem/${problem_id}/sample" \
    "${delete_sample_response_file}" \
    "200" \
    "delete sample" \
    "${sign_up_token}"
assert_json_message \
    "${delete_sample_response_file}" \
    "problem sample deleted" \
    "delete sample"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}/sample" \
    "${remaining_samples_response_file}" \
    "200" \
    "list samples after delete"

python3 - "${remaining_samples_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

samples = response.get("samples")
if response.get("sample_count") != 1 or not isinstance(samples, list) or len(samples) != 1:
    raise SystemExit("unexpected remaining sample list after delete")

remaining_sample = samples[0]
if remaining_sample.get("sample_order") != 1:
    raise SystemExit("unexpected remaining sample_order after delete")
if remaining_sample.get("sample_input") != "10 20\n":
    raise SystemExit("unexpected remaining sample_input after delete")
if remaining_sample.get("sample_output") != "30\n":
    raise SystemExit("unexpected remaining sample_output after delete")
PY

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/problem/${problem_id}" \
    "${deleted_problem_response_file}" \
    "200" \
    "get problem after sample delete"

if ! python3 - "${deleted_problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
expected_problem_id = int(sys.argv[2])

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch after sample delete")
if response.get("version") != 3:
    raise SystemExit("version mismatch after sample delete")
if response.get("sample_count") != 1:
    raise SystemExit("sample_count mismatch after sample delete")

expected_samples = [
    {
        "sample_order": 1,
        "sample_input": "10 20\n",
        "sample_output": "30\n",
    },
]
if response.get("samples") != expected_samples:
    raise SystemExit("public samples mismatch after sample delete")
PY
then
    append_log_line "${test_log_temp_file}" "deleted public sample verification failed"
    publish_failure_logs
    exit 1
fi

append_log_line "${test_log_temp_file}" "problem sample flow test passed"
print_success_log "problem sample flow test passed: problem_id=${problem_id}, admin_user_id=${sign_up_user_id}"
