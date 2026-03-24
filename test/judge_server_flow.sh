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

http_port="${JUDGE_SERVER_FLOW_TEST_HTTP_PORT:-18085}"
base_url="${JUDGE_SERVER_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${JUDGE_SERVER_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
judge_server_bin="${JUDGE_SERVER_FLOW_TEST_JUDGE_SERVER_BIN:-${project_root}/judge_server}"
user_login_id="${JUDGE_SERVER_FLOW_TEST_LOGIN_ID:-judge_server_flow_test_$(date +%s)_$$}"
raw_password="${JUDGE_SERVER_FLOW_TEST_PASSWORD:-password123}"
test_db_name="judge_server_flow_test_$$_$(date +%s)"
test_database_created="0"
problem_memory_limit_mb="${JUDGE_SERVER_FLOW_TEST_MEMORY_LIMIT_MB:-256}"
problem_time_limit_ms="${JUDGE_SERVER_FLOW_TEST_TIME_LIMIT_MS:-2000}"
accepted_source_code="${JUDGE_SERVER_FLOW_TEST_ACCEPTED_SOURCE_CODE:-#include <chrono>
#include <iostream>
#include <thread>
int main(){
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    std::cout << 3 << '\n';
    return 0;
}
}"
wrong_answer_source_code="${JUDGE_SERVER_FLOW_TEST_WRONG_ANSWER_SOURCE_CODE:-#include <iostream>
int main(){
    std::cout << 4 << '\n';
    return 0;
}
}"
compile_error_source_code="${JUDGE_SERVER_FLOW_TEST_COMPILE_ERROR_SOURCE_CODE:-#include <iostream>
int main(){
    std::cout << 3 << '\n'
    return 0;
}
}"
runtime_error_source_code="${JUDGE_SERVER_FLOW_TEST_RUNTIME_ERROR_SOURCE_CODE:-#include <cstdlib>
#include <iostream>
int main(){
    std::cerr << \"runtime boom\\n\";
    std::abort();
}
}"
time_limit_exceeded_source_code="${JUDGE_SERVER_FLOW_TEST_TIME_LIMIT_EXCEEDED_SOURCE_CODE:-#include <chrono>
#include <thread>
int main(){
    std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    return 0;
}
}"
judge_source_root="$(mktemp -d)"
testcase_root="$(mktemp -d)"
test_log_path=""
server_log_path=""
judge_server_log_path=""
server_pid=""
judge_server_pid=""
test_log_name="test_judge_server_flow.log"
server_log_name="test_judge_server_flow_http_server.log"
judge_server_log_name="test_judge_server_flow_judge_server.log"
test_log_temp_file="$(mktemp)"
server_log_temp_file="$(mktemp)"
judge_server_log_temp_file="$(mktemp)"
sign_up_response_file="$(mktemp)"
create_problem_response_file="$(mktemp)"
set_limits_response_file="$(mktemp)"
create_testcase_response_file="$(mktemp)"
submission_response_file="$(mktemp)"
accepted_submission_detail_response_file="$(mktemp)"
wrong_answer_submission_detail_response_file="$(mktemp)"
compile_error_submission_detail_response_file="$(mktemp)"
runtime_error_submission_detail_response_file="$(mktemp)"
time_limit_exceeded_submission_detail_response_file="$(mktemp)"
problem_response_file="$(mktemp)"
submission_source_response_file="$(mktemp)"
submission_history_response_file="$(mktemp)"

cleanup_judge_server(){
    if [[ -n "${judge_server_pid:-}" ]]; then
        kill "${judge_server_pid}" >/dev/null 2>&1 || true
        wait "${judge_server_pid}" >/dev/null 2>&1 || true
    fi
}

cleanup(){
    cleanup_judge_server
    cleanup_http_server
    drop_test_database

    rm -rf "${judge_source_root}" "${testcase_root}"
    rm -f \
        "${test_log_temp_file}" \
        "${server_log_temp_file}" \
        "${judge_server_log_temp_file}" \
        "${sign_up_response_file}" \
        "${create_problem_response_file}" \
        "${set_limits_response_file}" \
        "${create_testcase_response_file}" \
        "${submission_response_file}" \
        "${accepted_submission_detail_response_file}" \
        "${wrong_answer_submission_detail_response_file}" \
        "${compile_error_submission_detail_response_file}" \
        "${runtime_error_submission_detail_response_file}" \
        "${time_limit_exceeded_submission_detail_response_file}" \
        "${problem_response_file}" \
        "${submission_source_response_file}" \
        "${submission_history_response_file}"
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

publish_judge_server_failure_log(){
    if [[ -z "${judge_server_log_path:-}" ]] && [[ -n "${judge_server_pid:-}" || -s "${judge_server_log_temp_file}" ]]; then
        judge_server_log_path="$(
            publish_log_file "${judge_server_log_temp_file}" "${judge_server_log_name}"
        )"
        print_log_file_created "${judge_server_log_path}"
    fi
}

publish_all_failure_logs(){
    publish_failure_logs
    publish_judge_server_failure_log
}

ensure_judge_server(){
    if [[ ! -x "${judge_server_bin}" ]]; then
        echo "judge_server binary not found or not executable: ${judge_server_bin}" >&2
        append_log_line "${test_log_temp_file}" "judge_server binary not found: ${judge_server_bin}"
        publish_all_failure_logs
        echo "hint: run 'cmake --build ${project_root}/build'" >&2
        return 1
    fi

    append_log_line "${test_log_temp_file}" "starting local judge_server"
    "${judge_server_bin}" >"${judge_server_log_temp_file}" 2>&1 &
    judge_server_pid="$!"

    local attempt=0
    while (( attempt < 20 )); do
        if ! kill -0 "${judge_server_pid}" >/dev/null 2>&1; then
            echo "failed to start judge_server" >&2
            append_log_line "${test_log_temp_file}" "failed to start local judge_server"
            publish_all_failure_logs
            echo "judge server log:" >&2
            cat "${judge_server_log_temp_file}" >&2
            return 1
        fi

        if (( attempt >= 2 )); then
            return 0
        fi

        sleep 0.1
        attempt=$((attempt + 1))
    done

    return 0
}

make_limits_request_body(){
    python3 - "$1" "$2" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "memory_limit_mb": int(sys.argv[1]),
            "time_limit_ms": int(sys.argv[2]),
        }
    )
)
PY
}

make_testcase_request_body(){
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
}

make_submission_request_body(){
    python3 - "$1" "$2" <<'PY'
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
}

wait_for_submission_final_status(){
    local submission_id="$1"
    local expected_status="$2"
    local response_file_path="$3"
    local attempt=0

    while (( attempt < 120 )); do
        local detail_status_code=""
        detail_status_code="$(
            send_http_request \
                "GET" \
                "${base_url}/api/submission/${submission_id}" \
                "${response_file_path}"
        )"

        if [[ "${detail_status_code}" != "200" ]]; then
            append_log_line "${test_log_temp_file}" "submission detail poll failed: submission_id=${submission_id}, status=${detail_status_code}"
            publish_all_failure_logs
            echo "submission detail poll failed for submission ${submission_id}: expected status 200, got ${detail_status_code}" >&2
            echo "response body:" >&2
            cat "${response_file_path}" >&2
            exit 1
        fi

        local current_status=""
        current_status="$(
            python3 - "${response_file_path}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

status = response.get("status")
if not isinstance(status, str) or not status:
    raise SystemExit("missing status in submission detail response")

print(status)
PY
        )"

        append_log_line "${test_log_temp_file}" "submission status poll: submission_id=${submission_id}, attempt=${attempt}, status=${current_status}"

        if [[ "${current_status}" == "${expected_status}" ]]; then
            return 0
        fi

        if [[ "${current_status}" != "queued" && "${current_status}" != "judging" ]]; then
            append_log_line "${test_log_temp_file}" "unexpected submission status: submission_id=${submission_id}, status=${current_status}"
            publish_all_failure_logs
            echo "unexpected submission status for submission ${submission_id}: ${current_status}" >&2
            exit 1
        fi

        sleep 0.1
        attempt=$((attempt + 1))
    done

    append_log_line "${test_log_temp_file}" "submission final status timeout: submission_id=${submission_id}, expected_status=${expected_status}"
    publish_all_failure_logs
    echo "timed out waiting for submission ${submission_id} to reach ${expected_status}" >&2
    echo "last response body:" >&2
    cat "${response_file_path}" >&2
    exit 1
}

validate_submission_detail(){
    local response_file_path="$1"
    local expected_submission_id="$2"
    local expected_user_id="$3"
    local expected_problem_id="$4"
    local expected_language="$5"
    local expected_status="$6"
    local expected_score="$7"
    local compile_output_expectation="$8"
    local judge_output_expectation="$9"
    local elapsed_ms_expectation="${10}"
    local max_rss_kb_expectation="${11}"

    python3 \
        - "${response_file_path}" \
        "${expected_submission_id}" \
        "${expected_user_id}" \
        "${expected_problem_id}" \
        "${expected_language}" \
        "${expected_status}" \
        "${expected_score}" \
        "${compile_output_expectation}" \
        "${judge_output_expectation}" \
        "${elapsed_ms_expectation}" \
        "${max_rss_kb_expectation}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_user_id = int(sys.argv[3])
expected_problem_id = int(sys.argv[4])
expected_language = sys.argv[5]
expected_status = sys.argv[6]
expected_score = int(sys.argv[7])
compile_output_expectation = sys.argv[8]
judge_output_expectation = sys.argv[9]
elapsed_ms_expectation = sys.argv[10]
max_rss_kb_expectation = sys.argv[11]

if response.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch")
if response.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch")
if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch")
if response.get("language") != expected_language:
    raise SystemExit("language mismatch")
if response.get("status") != expected_status:
    raise SystemExit("status mismatch")
if response.get("score") != expected_score:
    raise SystemExit("score mismatch")

compile_output = response.get("compile_output", "__missing__")
judge_output = response.get("judge_output", "__missing__")
elapsed_ms = response.get("elapsed_ms", "__missing__")
max_rss_kb = response.get("max_rss_kb", "__missing__")

if compile_output_expectation == "null":
    if compile_output is not None:
        raise SystemExit("expected compile_output to be null")
elif compile_output_expectation == "non_empty":
    if not isinstance(compile_output, str) or not compile_output:
        raise SystemExit("expected compile_output to be a non-empty string")
else:
    raise SystemExit("invalid compile_output expectation")

if judge_output_expectation == "null":
    if judge_output is not None:
        raise SystemExit("expected judge_output to be null")
elif judge_output_expectation == "non_empty":
    if not isinstance(judge_output, str) or not judge_output:
        raise SystemExit("expected judge_output to be a non-empty string")
else:
    raise SystemExit("invalid judge_output expectation")

if elapsed_ms_expectation == "null":
    if elapsed_ms is not None:
        raise SystemExit("expected elapsed_ms to be null")
elif elapsed_ms_expectation == "non_negative_int":
    if not isinstance(elapsed_ms, int) or elapsed_ms < 0:
        raise SystemExit("expected elapsed_ms to be a non-negative integer")
else:
    raise SystemExit("invalid elapsed_ms expectation")

if max_rss_kb_expectation == "null":
    if max_rss_kb is not None:
        raise SystemExit("expected max_rss_kb to be null")
elif max_rss_kb_expectation == "non_negative_int":
    if not isinstance(max_rss_kb, int) or max_rss_kb < 0:
        raise SystemExit("expected max_rss_kb to be a non-negative integer")
else:
    raise SystemExit("invalid max_rss_kb expectation")

created_at = response.get("created_at")
updated_at = response.get("updated_at")
if not isinstance(created_at, str) or not created_at:
    raise SystemExit("missing created_at")
if not isinstance(updated_at, str) or not updated_at:
    raise SystemExit("missing updated_at")

if "source_code" in response:
    raise SystemExit("submission detail must not expose source_code")
PY
}

validate_submission_status_history(){
    local submission_id="$1"
    local expected_final_status="$2"

    local submission_history_status_code=""
    submission_history_status_code="$(
        send_http_request \
            "GET" \
            "${base_url}/api/submission/${submission_id}/history" \
            "${submission_history_response_file}"
    )"

    if [[ "${submission_history_status_code}" != "200" ]]; then
        append_log_line "${test_log_temp_file}" "submission history get failed: submission_id=${submission_id}, status=${submission_history_status_code}"
        publish_all_failure_logs
        echo "submission history get failed for submission ${submission_id}: expected status 200, got ${submission_history_status_code}" >&2
        echo "response body:" >&2
        cat "${submission_history_response_file}" >&2
        exit 1
    fi

    python3 \
        - "${submission_history_response_file}" \
        "${submission_id}" \
        "${expected_final_status}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_final_status = sys.argv[3]

if response.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in submission history response")

if response.get("history_count") != 3:
    raise SystemExit("expected history_count to be 3 in submission history response")

histories = response.get("histories")
if not isinstance(histories, list) or len(histories) != 3:
    raise SystemExit("expected three history rows in submission history response")

expected_pairs = [
    (None, "queued"),
    ("queued", "judging"),
    ("judging", expected_final_status),
]

for history, expected_pair in zip(histories, expected_pairs):
    history_id = history.get("history_id")
    if not isinstance(history_id, int) or history_id <= 0:
        raise SystemExit("invalid history_id in submission history response")

    if history.get("from_status", "missing") != expected_pair[0]:
        raise SystemExit("from_status mismatch in submission history response")

    if history.get("to_status") != expected_pair[1]:
        raise SystemExit("to_status mismatch in submission history response")

    if history.get("reason", "missing") is not None:
        raise SystemExit("expected reason to be null in submission history response")

    created_at = history.get("created_at")
    if not isinstance(created_at, str) or not created_at:
        raise SystemExit("missing created_at in submission history response")
PY
}

validate_submission_source(){
    local response_file_path="$1"
    local expected_submission_id="$2"
    local expected_user_id="$3"
    local expected_problem_id="$4"
    local expected_language="$5"
    local expected_source_code="$6"
    local compile_output_expectation="$7"
    local judge_output_expectation="$8"

    python3 \
        - "${response_file_path}" \
        "${expected_submission_id}" \
        "${expected_user_id}" \
        "${expected_problem_id}" \
        "${expected_language}" \
        "${expected_source_code}" \
        "${compile_output_expectation}" \
        "${judge_output_expectation}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

expected_submission_id = int(sys.argv[2])
expected_user_id = int(sys.argv[3])
expected_problem_id = int(sys.argv[4])
expected_language = sys.argv[5]
expected_source_code = sys.argv[6]
compile_output_expectation = sys.argv[7]
judge_output_expectation = sys.argv[8]

if response.get("submission_id") != expected_submission_id:
    raise SystemExit("submission_id mismatch in submission source response")
if response.get("user_id") != expected_user_id:
    raise SystemExit("user_id mismatch in submission source response")
if response.get("problem_id") != expected_problem_id:
    raise SystemExit("problem_id mismatch in submission source response")
if response.get("language") != expected_language:
    raise SystemExit("language mismatch in submission source response")
if response.get("source_code") != expected_source_code:
    raise SystemExit("source_code mismatch in submission source response")

compile_output = response.get("compile_output", "__missing__")
judge_output = response.get("judge_output", "__missing__")

if compile_output_expectation == "null":
    if compile_output is not None:
        raise SystemExit("expected compile_output to be null in submission source response")
elif compile_output_expectation == "non_empty":
    if not isinstance(compile_output, str) or not compile_output:
        raise SystemExit("expected compile_output to be a non-empty string in submission source response")
else:
    raise SystemExit("invalid compile_output expectation for submission source response")

if judge_output_expectation == "null":
    if judge_output is not None:
        raise SystemExit("expected judge_output to be null in submission source response")
elif judge_output_expectation == "non_empty":
    if not isinstance(judge_output, str) or not judge_output:
        raise SystemExit("expected judge_output to be a non-empty string in submission source response")
else:
    raise SystemExit("invalid judge_output expectation for submission source response")
PY
}

trap cleanup EXIT

require_command curl
require_command psql
require_command python3

cpp_compiler_path="$(command -v c++ || true)"
if [[ -z "${cpp_compiler_path}" ]]; then
    cpp_compiler_path="$(command -v g++ || true)"
fi
if [[ -z "${cpp_compiler_path}" ]]; then
    echo "missing c++ compiler for judge flow test" >&2
    exit 1
fi

python_path="$(command -v python3)"
java_runtime_path="$(command -v java || true)"
if [[ -z "${java_runtime_path}" ]]; then
    java_runtime_path="/bin/true"
fi

export DB_ADMIN_USER="${DB_ADMIN_USER:-${DB_USER:-postgres}}"
export DB_ADMIN_PASSWORD="${DB_ADMIN_PASSWORD:-${DB_PASSWORD:-postgres}}"
create_test_database
test_database_url="postgresql://${DB_USER}:${DB_PASSWORD}@${DB_HOST}:${DB_PORT}/${test_db_name}"
export DB_NAME="${test_db_name}"
export HTTP_PORT="${http_port}"
export WORKER_ID="judge_server_flow_$$"
export MAX_CONCURRENT_JOBS="${MAX_CONCURRENT_JOBS:-1}"
export JUDGE_SOURCE_ROOT="${judge_source_root}"
export TESTCASE_PATH="${testcase_root}"
export JUDGE_CPP_COMPILER_PATH="${cpp_compiler_path}"
export JUDGE_PYTHON_PATH="${python_path}"
export JUDGE_JAVA_RUNTIME_PATH="${java_runtime_path}"

append_log_line "${test_log_temp_file}" "base_url=${base_url}"
append_log_line "${test_log_temp_file}" "user_login_id=${user_login_id}"
append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"
append_log_line "${test_log_temp_file}" "judge_source_root=${judge_source_root}"
append_log_line "${test_log_temp_file}" "testcase_root=${testcase_root}"

apply_test_database_migrations
ensure_dedicated_http_server

read -r sign_up_user_id sign_up_token < <(
    sign_up_user "${user_login_id}" "${raw_password}" "${sign_up_response_file}" "judge server flow"
)

print_success_log "sign-up success"

promote_admin_user "${sign_up_user_id}" "judge server flow" >/dev/null
print_success_log "admin promotion success"

problem_id="$(
    create_problem_via_api "${sign_up_token}" "${create_problem_response_file}" "judge server flow"
)"

print_success_log "problem create success"

limits_request_body="$(
    make_limits_request_body "${problem_memory_limit_mb}" "${problem_time_limit_ms}"
)"
set_limits_status_code="$(
    send_http_request \
        "PUT" \
        "${base_url}/api/problem/${problem_id}/limits" \
        "${set_limits_response_file}" \
        "${sign_up_token}" \
        "${limits_request_body}"
)"
assert_status_code "${set_limits_status_code}" "200" "${set_limits_response_file}" "set limits"

append_log_line "${test_log_temp_file}" "problem limits updated: problem_id=${problem_id}, memory_limit_mb=${problem_memory_limit_mb}, time_limit_ms=${problem_time_limit_ms}"
print_success_log "problem limits update success"

testcase_request_body="$(make_testcase_request_body)"
create_testcase_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/problem/${problem_id}/testcase" \
        "${create_testcase_response_file}" \
        "${sign_up_token}" \
        "${testcase_request_body}"
)"
assert_status_code "${create_testcase_status_code}" "201" "${create_testcase_response_file}" "create testcase"

python3 - "${create_testcase_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

testcase_order = response.get("testcase_order")
if testcase_order != 1:
    raise SystemExit("unexpected testcase_order after testcase create")
PY

append_log_line "${test_log_temp_file}" "testcase created: problem_id=${problem_id}, testcase_order=1"
print_success_log "problem testcase create success"

ensure_judge_server
print_success_log "judge server start success"

accepted_submission_request_body="$(
    make_submission_request_body "cpp" "${accepted_source_code}"
)"
accepted_submission_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${submission_response_file}" \
        "${sign_up_token}" \
        "${accepted_submission_request_body}"
)"
assert_status_code "${accepted_submission_status_code}" "201" "${submission_response_file}" "accepted submission create"

accepted_submission_id="$(
    python3 - "${submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

submission_id = response.get("submission_id")
status = response.get("status")
if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid submission_id in accepted submission response")
if status != "queued":
    raise SystemExit("expected accepted submission status to be queued")

print(submission_id)
PY
)"

append_log_line "${test_log_temp_file}" "accepted submission created: submission_id=${accepted_submission_id}"
wait_for_submission_final_status \
    "${accepted_submission_id}" \
    "accepted" \
    "${accepted_submission_detail_response_file}"
validate_submission_detail \
    "${accepted_submission_detail_response_file}" \
    "${accepted_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "cpp" \
    "accepted" \
    "100" \
    "null" \
    "null" \
    "non_negative_int" \
    "non_negative_int"
validate_submission_status_history "${accepted_submission_id}" "accepted"
print_success_log "accepted submission judged successfully"

wrong_answer_submission_request_body="$(
    make_submission_request_body "cpp" "${wrong_answer_source_code}"
)"
wrong_answer_submission_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${submission_response_file}" \
        "${sign_up_token}" \
        "${wrong_answer_submission_request_body}"
)"
assert_status_code "${wrong_answer_submission_status_code}" "201" "${submission_response_file}" "wrong_answer submission create"

wrong_answer_submission_id="$(
    python3 - "${submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

submission_id = response.get("submission_id")
status = response.get("status")
if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid submission_id in wrong_answer submission response")
if status != "queued":
    raise SystemExit("expected wrong_answer submission status to be queued")

print(submission_id)
PY
)"

append_log_line "${test_log_temp_file}" "wrong_answer submission created: submission_id=${wrong_answer_submission_id}"
wait_for_submission_final_status \
    "${wrong_answer_submission_id}" \
    "wrong_answer" \
    "${wrong_answer_submission_detail_response_file}"
validate_submission_detail \
    "${wrong_answer_submission_detail_response_file}" \
    "${wrong_answer_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "cpp" \
    "wrong_answer" \
    "0" \
    "null" \
    "null" \
    "non_negative_int" \
    "non_negative_int"
validate_submission_status_history "${wrong_answer_submission_id}" "wrong_answer"
print_success_log "wrong_answer submission judged successfully"

compile_error_submission_request_body="$(
    make_submission_request_body "cpp" "${compile_error_source_code}"
)"
compile_error_submission_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${submission_response_file}" \
        "${sign_up_token}" \
        "${compile_error_submission_request_body}"
)"
assert_status_code "${compile_error_submission_status_code}" "201" "${submission_response_file}" "compile_error submission create"

compile_error_submission_id="$(
    python3 - "${submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

submission_id = response.get("submission_id")
status = response.get("status")
if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid submission_id in compile_error submission response")
if status != "queued":
    raise SystemExit("expected compile_error submission status to be queued")

print(submission_id)
PY
)"

append_log_line "${test_log_temp_file}" "compile_error submission created: submission_id=${compile_error_submission_id}"
wait_for_submission_final_status \
    "${compile_error_submission_id}" \
    "compile_error" \
    "${compile_error_submission_detail_response_file}"
validate_submission_detail \
    "${compile_error_submission_detail_response_file}" \
    "${compile_error_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "cpp" \
    "compile_error" \
    "0" \
    "non_empty" \
    "null" \
    "null" \
    "null"
validate_submission_status_history "${compile_error_submission_id}" "compile_error"

compile_error_submission_source_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${compile_error_submission_id}/source" \
        "${submission_source_response_file}" \
        "${sign_up_token}"
)"
assert_status_code "${compile_error_submission_source_status_code}" "200" "${submission_source_response_file}" "compile_error submission source get"

validate_submission_source \
    "${submission_source_response_file}" \
    "${compile_error_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "cpp" \
    "${compile_error_source_code}" \
    "non_empty" \
    "null"
print_success_log "compile_error submission judged successfully"

runtime_error_submission_request_body="$(
    make_submission_request_body "cpp" "${runtime_error_source_code}"
)"
runtime_error_submission_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${submission_response_file}" \
        "${sign_up_token}" \
        "${runtime_error_submission_request_body}"
)"
assert_status_code "${runtime_error_submission_status_code}" "201" "${submission_response_file}" "runtime_error submission create"

runtime_error_submission_id="$(
    python3 - "${submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

submission_id = response.get("submission_id")
status = response.get("status")
if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid submission_id in runtime_error submission response")
if status != "queued":
    raise SystemExit("expected runtime_error submission status to be queued")

print(submission_id)
PY
)"

append_log_line "${test_log_temp_file}" "runtime_error submission created: submission_id=${runtime_error_submission_id}"
wait_for_submission_final_status \
    "${runtime_error_submission_id}" \
    "runtime_error" \
    "${runtime_error_submission_detail_response_file}"
validate_submission_detail \
    "${runtime_error_submission_detail_response_file}" \
    "${runtime_error_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "cpp" \
    "runtime_error" \
    "0" \
    "null" \
    "non_empty" \
    "non_negative_int" \
    "non_negative_int"
validate_submission_status_history "${runtime_error_submission_id}" "runtime_error"

runtime_error_submission_source_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/submission/${runtime_error_submission_id}/source" \
        "${submission_source_response_file}" \
        "${sign_up_token}"
)"
assert_status_code "${runtime_error_submission_source_status_code}" "200" "${submission_source_response_file}" "runtime_error submission source get"

validate_submission_source \
    "${submission_source_response_file}" \
    "${runtime_error_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "cpp" \
    "${runtime_error_source_code}" \
    "null" \
    "non_empty"
print_success_log "runtime_error submission judged successfully"

time_limit_exceeded_submission_request_body="$(
    make_submission_request_body "cpp" "${time_limit_exceeded_source_code}"
)"
time_limit_exceeded_submission_status_code="$(
    send_http_request \
        "POST" \
        "${base_url}/api/submission/${problem_id}" \
        "${submission_response_file}" \
        "${sign_up_token}" \
        "${time_limit_exceeded_submission_request_body}"
)"
assert_status_code "${time_limit_exceeded_submission_status_code}" "201" "${submission_response_file}" "time_limit_exceeded submission create"

time_limit_exceeded_submission_id="$(
    python3 - "${submission_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

submission_id = response.get("submission_id")
status = response.get("status")
if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid submission_id in time_limit_exceeded submission response")
if status != "queued":
    raise SystemExit("expected time_limit_exceeded submission status to be queued")

print(submission_id)
PY
)"

append_log_line "${test_log_temp_file}" "time_limit_exceeded submission created: submission_id=${time_limit_exceeded_submission_id}"
wait_for_submission_final_status \
    "${time_limit_exceeded_submission_id}" \
    "time_limit_exceeded" \
    "${time_limit_exceeded_submission_detail_response_file}"
validate_submission_detail \
    "${time_limit_exceeded_submission_detail_response_file}" \
    "${time_limit_exceeded_submission_id}" \
    "${sign_up_user_id}" \
    "${problem_id}" \
    "cpp" \
    "time_limit_exceeded" \
    "0" \
    "null" \
    "null" \
    "non_negative_int" \
    "non_negative_int"
validate_submission_status_history "${time_limit_exceeded_submission_id}" "time_limit_exceeded"
print_success_log "time_limit_exceeded submission judged successfully"

problem_status_code="$(
    send_http_request \
        "GET" \
        "${base_url}/api/problem/${problem_id}" \
        "${problem_response_file}"
)"
assert_status_code "${problem_status_code}" "200" "${problem_response_file}" "problem statistics get"

python3 - "${problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch in problem detail response")

statistics = response.get("statistics")
if statistics != {"submission_count": 5, "accepted_count": 1}:
    raise SystemExit("problem statistics mismatch after judge flow")
PY

print_success_log "problem statistics validated"
append_log_line "${test_log_temp_file}" "judge server flow test passed"
print_success_log "judge server flow test passed: problem_id=${problem_id}, accepted_submission_id=${accepted_submission_id}, wrong_answer_submission_id=${wrong_answer_submission_id}, compile_error_submission_id=${compile_error_submission_id}, runtime_error_submission_id=${runtime_error_submission_id}, time_limit_exceeded_submission_id=${time_limit_exceeded_submission_id}"
