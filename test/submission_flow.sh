#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/util.sh"
# shellcheck disable=SC1091
source "${script_dir}/http_server_util.sh"

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
second_submission_source_code="${SUBMISSION_FLOW_TEST_SECOND_SOURCE_CODE:-#include <iostream>
int main(){
    std::cout << 1 << '\n';
    return 0;
}
}"
all_submission_response_file="$(mktemp)"
submission_detail_response_file="$(mktemp)"
missing_submission_response_file="$(mktemp)"
list_submission_response_file="$(mktemp)"
top_submission_response_file="$(mktemp)"
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
        "${all_submission_response_file}" \
        "${submission_detail_response_file}" \
        "${missing_submission_response_file}" \
        "${list_submission_response_file}" \
        "${top_submission_response_file}"

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

second_submission_request_body="$(
    python3 - "${submission_language}" "${second_submission_source_code}" <<'PY'
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

second_submission_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${submission_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${second_submission_request_body}" \
        "${base_url}/api/submission/${problem_id}"
)"

if [[ "${second_submission_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "second submission create failed: status=${second_submission_status_code}"
    publish_failure_logs
    echo "second submission create test failed: expected status 201, got ${second_submission_status_code}" >&2
    echo "response body:" >&2
    cat "${submission_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "second submission create passed: status=${second_submission_status_code}"
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

submission_detail_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${submission_detail_response_file}" \
        --write-out "%{http_code}" \
        --request GET \
        "${base_url}/api/submission/${submission_id}"
)"

if [[ "${submission_detail_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "submission detail get failed: status=${submission_detail_status_code}"
    publish_failure_logs
    echo "submission detail get test failed: expected status 200, got ${submission_detail_status_code}" >&2
    echo "response body:" >&2
    cat "${submission_detail_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "submission detail get passed: status=${submission_detail_status_code}"
print_success_log "submission detail get success"

missing_submission_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${missing_submission_response_file}" \
        --write-out "%{http_code}" \
        --request GET \
        "${base_url}/api/submission/${missing_submission_id}"
)"

if [[ "${missing_submission_status_code}" != "404" ]]; then
    append_log_line "${test_log_temp_file}" "missing submission get failed: status=${missing_submission_status_code}"
    publish_failure_logs
    echo "missing submission get test failed: expected status 404, got ${missing_submission_status_code}" >&2
    echo "response body:" >&2
    cat "${missing_submission_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "missing submission get passed: status=${missing_submission_status_code}"
print_success_log "missing submission get success"

all_submission_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${all_submission_response_file}" \
        --write-out "%{http_code}" \
        --request GET \
        "${base_url}/api/submission"
)"

if [[ "${all_submission_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "submission list without filter failed: status=${all_submission_status_code}"
    publish_failure_logs
    echo "submission list without filter test failed: expected status 200, got ${all_submission_status_code}" >&2
    echo "response body:" >&2
    cat "${all_submission_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "submission list without filter passed: status=${all_submission_status_code}"
print_success_log "submission list without filter success"

list_submission_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${list_submission_response_file}" \
        --write-out "%{http_code}" \
        --request GET \
        "${base_url}/api/submission?top=${second_submission_id}&user_id=${sign_up_user_id}&problem_id=${problem_id}&status=queued"
)"

if [[ "${list_submission_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "submission list failed: status=${list_submission_status_code}"
    publish_failure_logs
    echo "submission list test failed: expected status 200, got ${list_submission_status_code}" >&2
    echo "response body:" >&2
    cat "${list_submission_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "submission list passed: status=${list_submission_status_code}"
print_success_log "submission list success"

top_submission_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${top_submission_response_file}" \
        --write-out "%{http_code}" \
        --request GET \
        "${base_url}/api/submission?top=${submission_id}&user_id=${sign_up_user_id}&problem_id=${problem_id}&status=queued"
)"

if [[ "${top_submission_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "submission top list failed: status=${top_submission_status_code}"
    publish_failure_logs
    echo "submission top list test failed: expected status 200, got ${top_submission_status_code}" >&2
    echo "response body:" >&2
    cat "${top_submission_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "submission top list passed: status=${top_submission_status_code}"
print_success_log "submission top list success"

if ! python3 \
    - "${all_submission_response_file}" \
    "${submission_id}" \
    "${second_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "${submission_language}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_list = json.load(response_file)

expected_first_submission_id = int(sys.argv[2])
expected_second_submission_id = int(sys.argv[3])
expected_user_id = int(sys.argv[4])
expected_problem_id = int(sys.argv[5])
expected_language = sys.argv[6]

submission_count = submission_list.get("submission_count")
if not isinstance(submission_count, int) or submission_count < 2:
    raise SystemExit("expected submission_count to be at least 2")

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) < 2:
    raise SystemExit("expected at least two submissions in unfiltered submission list response")

latest_submission = submissions[0]
older_submission = submissions[1]

if latest_submission.get("submission_id") != expected_second_submission_id:
    raise SystemExit("expected newest submission to appear first in unfiltered list")

if older_submission.get("submission_id") != expected_first_submission_id:
    raise SystemExit("expected second newest submission to appear second in unfiltered list")

for submission in (latest_submission, older_submission):
    if submission.get("user_id") != expected_user_id:
        raise SystemExit("user_id mismatch in unfiltered submission list response")

    if submission.get("problem_id") != expected_problem_id:
        raise SystemExit("problem_id mismatch in unfiltered submission list response")

    if submission.get("language") != expected_language:
        raise SystemExit("language mismatch in unfiltered submission list response")

    if submission.get("status") != "queued":
        raise SystemExit("expected unfiltered submission list status to be queued")

    if submission.get("score", "missing") is not None:
        raise SystemExit("expected unfiltered submission list score to be null before judging")

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
    "${submission_language}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    submission_list = json.load(response_file)

expected_first_submission_id = int(sys.argv[2])
expected_second_submission_id = int(sys.argv[3])
expected_user_id = int(sys.argv[4])
expected_problem_id = int(sys.argv[5])
expected_language = sys.argv[6]

if submission_list.get("submission_count") != 2:
    raise SystemExit("expected submission_count to be 2")

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) != 2:
    raise SystemExit("expected two submissions in submission list response")

latest_submission = submissions[0]
older_submission = submissions[1]

if latest_submission.get("submission_id") != expected_second_submission_id:
    raise SystemExit("expected newest submission to appear first")

if older_submission.get("submission_id") != expected_first_submission_id:
    raise SystemExit("expected older submission to appear second")

for submission in submissions:
    if submission.get("user_id") != expected_user_id:
        raise SystemExit("user_id mismatch in submission list response")

    if submission.get("problem_id") != expected_problem_id:
        raise SystemExit("problem_id mismatch in submission list response")

    if submission.get("language") != expected_language:
        raise SystemExit("language mismatch in submission list response")

    if submission.get("status") != "queued":
        raise SystemExit("expected submission list status to be queued")

    if submission.get("score", "missing") is not None:
        raise SystemExit("expected submission list score to be null before judging")

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
    - "${top_submission_response_file}" \
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
    raise SystemExit("expected submission_count to be 1 for top filtered submission list response")

submissions = submission_list.get("submissions")
if not isinstance(submissions, list) or len(submissions) != 1:
    raise SystemExit("expected one submission in top filtered submission list response")

submission = submissions[0]

if submission.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in top filtered submission list response")

if submission.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch in top filtered submission list response")

if submission.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch in top filtered submission list response")

if submission.get("language") != expected_language:
    raise SystemExit("language mismatch in top filtered submission list response")

if submission.get("status") != "queued":
    raise SystemExit("expected top filtered submission list status to be queued")

if submission.get("score", "missing") is not None:
    raise SystemExit("expected top filtered submission list score to be null before judging")

if "source_code" in submission:
    raise SystemExit("top filtered submission list response must not expose source_code")
PY
then
    append_log_line "${test_log_temp_file}" "submission top list validation failed"
    publish_failure_logs
    exit 1
fi

append_log_line "${test_log_temp_file}" "submission flow test passed"
print_success_log "submission flow test passed: login_id=${user_login_id}, user_id=${sign_up_user_id}, problem_id=${problem_id}, submission_id=${submission_id}, second_submission_id=${second_submission_id}"
