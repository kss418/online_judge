#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/test_util.sh"
# shellcheck disable=SC1091
source "${script_dir}/http_server_test_util.sh"

if [[ -f "${project_root}/.env" ]]; then
    set -a
    # shellcheck disable=SC1091
    source "${project_root}/.env"
    set +a
fi

http_port="${HTTP_PORT:-18080}"
base_url="${SUBMISSION_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${SUBMISSION_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
user_login_id="${SUBMISSION_FLOW_TEST_LOGIN_ID:-submission_flow_test_$(date +%s)_$$}"
raw_password="${SUBMISSION_FLOW_TEST_PASSWORD:-password123}"
submission_language="${SUBMISSION_FLOW_TEST_LANGUAGE:-cpp}"
submission_source_code="${SUBMISSION_FLOW_TEST_SOURCE_CODE:-#include <iostream>
int main(){
    std::cout << 0 << '\n';
    return 0;
}
}"
get_submission_response_file="$(mktemp)"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_submission_flow.log"
server_log_name="test_submission_flow_server.log"
test_log_temp_file="$(mktemp)"
server_log_temp_file="$(mktemp)"
sign_up_response_file="$(mktemp)"
submission_response_file="$(mktemp)"

cleanup(){
    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${sign_up_response_file}" \
        "${submission_response_file}" \
        "${get_submission_response_file}"

    cleanup_http_server
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

create_problem(){
    if [[ -z "${DB_HOST:-}" || -z "${DB_PORT:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWORD:-}" || -z "${DB_NAME:-}" ]]; then
        echo "missing required db envs" >&2
        return 1
    fi

    PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v ON_ERROR_STOP=1 \
        -qAt <<'SQL' | sed -n '1p'
WITH created_problem AS (
    INSERT INTO problems(version)
    VALUES(1)
    RETURNING problem_id
)
INSERT INTO problem_statistics(problem_id)
SELECT problem_id
FROM created_problem
RETURNING problem_id;
SQL
}

trap cleanup EXIT

require_command curl
require_command psql
require_command python3

append_log_line "${test_log_temp_file}" "base_url=${base_url}"
append_log_line "${test_log_temp_file}" "user_login_id=${user_login_id}"

ensure_http_server

sign_up_request_body="$(
    python3 - "${user_login_id}" "${raw_password}" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "user_login_id": sys.argv[1],
            "raw_password": sys.argv[2],
        }
    )
)
PY
)"

sign_up_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${sign_up_response_file}" \
        --write-out "%{http_code}" \
        -H "Content-Type: application/json" \
        -d "${sign_up_request_body}" \
        "${base_url}/api/auth/sign-up"
)"

if [[ "${sign_up_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "sign-up failed: status=${sign_up_status_code}"
    publish_failure_logs
    echo "sign-up test failed: expected status 201, got ${sign_up_status_code}" >&2
    echo "response body:" >&2
    cat "${sign_up_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "sign-up passed: status=${sign_up_status_code}"
print_success_log "sign-up success"

sign_up_user_id="$(
    python3 - "${sign_up_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    sign_up_response = json.load(response_file)

user_id = sign_up_response.get("user_id")
if not isinstance(user_id, int) or user_id <= 0:
    raise SystemExit("invalid user_id in sign-up response")

print(user_id)
PY
)"

sign_up_token="$(
    python3 - "${sign_up_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    sign_up_response = json.load(response_file)

token = sign_up_response.get("token")
if not isinstance(token, str) or not token:
    raise SystemExit("missing token in sign-up response")

print(token)
PY
)"

problem_id="$(create_problem)"
if [[ -z "${problem_id}" ]]; then
    append_log_line "${test_log_temp_file}" "problem create failed"
    publish_failure_logs
    echo "problem create test failed" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "problem created: problem_id=${problem_id}"
print_success_log "problem create success"

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
    curl \
        --silent \
        --show-error \
        --output "${submission_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${submission_request_body}" \
        "${base_url}/api/submission/${problem_id}"
)"

if [[ "${submission_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "submission create failed: status=${submission_status_code}"
    publish_failure_logs
    echo "submission create test failed: expected status 201, got ${submission_status_code}" >&2
    echo "response body:" >&2
    cat "${submission_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "submission create passed: status=${submission_status_code}"
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

get_submission_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${get_submission_response_file}" \
        --write-out "%{http_code}" \
        --request GET \
        -H "Authorization: Bearer ${sign_up_token}" \
        "${base_url}/api/submission/${submission_id}"
)"

if [[ "${get_submission_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "submission get failed: status=${get_submission_status_code}"
    publish_failure_logs
    echo "submission get test failed: expected status 200, got ${get_submission_status_code}" >&2
    echo "response body:" >&2
    cat "${get_submission_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "submission get passed: status=${get_submission_status_code}"
print_success_log "submission get success"

if ! python3 \
    - "${get_submission_response_file}" \
    "${submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${submission_language}" \
    "${submission_source_code}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_detail = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_user_id = int(sys.argv[3])
expected_problem_id = int(sys.argv[4])
expected_language = sys.argv[5]
expected_source_code = sys.argv[6]

if submission_detail.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in submission get response")

if submission_detail.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch in submission get response")

if submission_detail.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch in submission get response")

if submission_detail.get("language") != expected_language:
    raise SystemExit("language mismatch in submission get response")

if submission_detail.get("source_code") != expected_source_code:
    raise SystemExit("source_code mismatch in submission get response")

if submission_detail.get("status") != "queued":
    raise SystemExit("expected submission get status to be queued")

if submission_detail.get("score", "missing") is not None:
    raise SystemExit("expected score to be null before judging")

if submission_detail.get("compile_output", "missing") is not None:
    raise SystemExit("expected compile_output to be null before judging")

if submission_detail.get("judge_output", "missing") is not None:
    raise SystemExit("expected judge_output to be null before judging")

created_at = submission_detail.get("created_at")
updated_at = submission_detail.get("updated_at")

if not isinstance(created_at, str) or not created_at:
    raise SystemExit("missing created_at in submission get response")

if not isinstance(updated_at, str) or not updated_at:
    raise SystemExit("missing updated_at in submission get response")
PY
then
    append_log_line "${test_log_temp_file}" "submission get validation failed"
    publish_failure_logs
    exit 1
fi

append_log_line "${test_log_temp_file}" "submission flow test passed"
print_success_log "submission flow test passed: login_id=${user_login_id}, user_id=${sign_up_user_id}, problem_id=${problem_id}, submission_id=${submission_id}"
