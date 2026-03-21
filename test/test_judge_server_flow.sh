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

http_port="${JUDGE_SERVER_FLOW_TEST_HTTP_PORT:-18080}"
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
problem_response_file="$(mktemp)"

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
        "${problem_response_file}"
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

require_db_env(){
    if [[ -z "${DB_HOST:-}" || -z "${DB_PORT:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWORD:-}" || -z "${DB_NAME:-}" ]]; then
        echo "missing required db envs" >&2
        exit 1
    fi
}

require_db_admin_env(){
    if [[ -z "${DB_ADMIN_USER:-}" || -z "${DB_ADMIN_PASSWORD:-}" ]]; then
        echo "missing required db admin envs" >&2
        exit 1
    fi
}

create_test_database(){
    require_db_env
    require_db_admin_env

    PGPASSWORD="${DB_ADMIN_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_ADMIN_USER}" \
        -d postgres \
        -v ON_ERROR_STOP=1 <<SQL >/dev/null
CREATE DATABASE "${test_db_name}" OWNER "${DB_USER}";
SQL

    test_database_created="1"
}

drop_test_database(){
    if [[ "${test_database_created}" != "1" ]]; then
        return 0
    fi

    if [[ -z "${DB_HOST:-}" || -z "${DB_PORT:-}" || -z "${DB_ADMIN_USER:-}" || -z "${DB_ADMIN_PASSWORD:-}" ]]; then
        return 0
    fi

    PGPASSWORD="${DB_ADMIN_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_ADMIN_USER}" \
        -d postgres \
        -v ON_ERROR_STOP=1 <<SQL >/dev/null 2>&1 || true
SELECT pg_terminate_backend(pid)
FROM pg_stat_activity
WHERE datname = '${test_db_name}' AND pid <> pg_backend_pid();

DROP DATABASE IF EXISTS "${test_db_name}";
SQL
}

run_schema_migration(){
    local migration_script_path="$1"
    local migration_name="$2"

    if ! DATABASE_URL="${test_database_url}" bash "${migration_script_path}" >>"${test_log_temp_file}" 2>&1; then
        append_log_line "${test_log_temp_file}" "migration failed: ${migration_name}"
        publish_all_failure_logs
        echo "failed to apply ${migration_name}" >&2
        return 1
    fi

    append_log_line "${test_log_temp_file}" "migration applied: ${migration_name}"
}

apply_test_database_migrations(){
    run_schema_migration "${project_root}/scripts/migrate_auth_schema.sh" "auth_schema"
    run_schema_migration "${project_root}/scripts/migrate_problem_schema.sh" "problem_schema"
    run_schema_migration "${project_root}/scripts/migrate_submission_schema.sh" "submission_schema"
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

ensure_dedicated_http_server(){
    require_http_server_test_env

    if health_check; then
        echo "http_server is already listening on ${base_url}" >&2
        append_log_line "${test_log_temp_file}" "refusing to reuse existing http_server: base_url=${base_url}"
        publish_failure_logs
        echo "set JUDGE_SERVER_FLOW_TEST_HTTP_PORT to an unused port and retry" >&2
        return 1
    fi

    if [[ ! -x "${http_server_bin}" ]]; then
        echo "http_server binary not found or not executable: ${http_server_bin}" >&2
        append_log_line "${test_log_temp_file}" "http_server binary not found: ${http_server_bin}"
        publish_failure_logs
        echo "hint: run 'cmake --build ${project_root}/build'" >&2
        return 1
    fi

    append_log_line "${test_log_temp_file}" "starting dedicated http_server"
    HTTP_PORT="${http_port}" "${http_server_bin}" >"${server_log_temp_file}" 2>&1 &
    server_pid="$!"

    if ! wait_for_health; then
        echo "failed to start dedicated http_server" >&2
        append_log_line "${test_log_temp_file}" "failed to start dedicated http_server"
        publish_failure_logs
        echo "server log:" >&2
        cat "${server_log_temp_file}" >&2
        return 1
    fi
}

make_sign_up_request_body(){
    python3 - "$1" "$2" <<'PY'
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

promote_admin(){
    local user_id="$1"

    require_db_env

    if [[ -z "${user_id}" ]]; then
        echo "missing user_id" >&2
        return 1
    fi

    PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v ON_ERROR_STOP=1 \
        -qAt <<SQL | sed -n '1p'
UPDATE users
SET is_admin = TRUE, updated_at = NOW()
WHERE user_id = ${user_id}
RETURNING user_id;
SQL
}

wait_for_submission_final_status(){
    local submission_id="$1"
    local expected_status="$2"
    local response_file_path="$3"
    local attempt=0

    while (( attempt < 120 )); do
        local detail_status_code=""
        detail_status_code="$(
            curl \
                --silent \
                --show-error \
                --output "${response_file_path}" \
                --write-out "%{http_code}" \
                --request GET \
                "${base_url}/api/submission/${submission_id}"
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

    python3 \
        - "${response_file_path}" \
        "${expected_submission_id}" \
        "${expected_user_id}" \
        "${expected_problem_id}" \
        "${expected_language}" \
        "${expected_status}" \
        "${expected_score}" \
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
expected_status = sys.argv[6]
expected_score = int(sys.argv[7])
compile_output_expectation = sys.argv[8]
judge_output_expectation = sys.argv[9]

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

    local actual_history=""
    actual_history="$(
        PGPASSWORD="${DB_PASSWORD}" psql \
            -X \
            -h "${DB_HOST}" \
            -p "${DB_PORT}" \
            -U "${DB_USER}" \
            -d "${DB_NAME}" \
            -v ON_ERROR_STOP=1 \
            -qAt <<SQL
SELECT COALESCE(from_status::text, 'null') || '->' || to_status::text
FROM submission_status_history
WHERE submission_id = ${submission_id}
ORDER BY history_id ASC;
SQL
    )"

    python3 - "${expected_final_status}" "${actual_history}" <<'PY'
import sys

expected_final_status = sys.argv[1]
actual_lines = [line for line in sys.argv[2].splitlines() if line]
expected_lines = [
    "null->queued",
    "queued->judging",
    f"judging->{expected_final_status}",
]

if actual_lines != expected_lines:
    raise SystemExit(
        "unexpected submission status history: "
        + repr(actual_lines)
        + " != "
        + repr(expected_lines)
    )
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

sign_up_request_body="$(make_sign_up_request_body "${user_login_id}" "${raw_password}")"
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
    publish_all_failure_logs
    echo "judge server flow sign-up failed: expected status 201, got ${sign_up_status_code}" >&2
    echo "response body:" >&2
    cat "${sign_up_response_file}" >&2
    exit 1
fi

read -r sign_up_user_id sign_up_token < <(
    python3 - "${sign_up_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

user_id = response.get("user_id")
token = response.get("token")
if not isinstance(user_id, int) or user_id <= 0:
    raise SystemExit("invalid user_id in sign-up response")
if not isinstance(token, str) or not token:
    raise SystemExit("invalid token in sign-up response")

print(user_id, token)
PY
)

print_success_log "sign-up success"

promoted_user_id="$(promote_admin "${sign_up_user_id}")"
if [[ "${promoted_user_id}" != "${sign_up_user_id}" ]]; then
    append_log_line "${test_log_temp_file}" "admin promotion failed: user_id=${sign_up_user_id}"
    publish_all_failure_logs
    echo "judge server flow admin promotion failed" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "admin promotion succeeded: user_id=${sign_up_user_id}"
print_success_log "admin promotion success"

create_problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${create_problem_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        "${base_url}/api/problem"
)"

if [[ "${create_problem_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "problem create failed: status=${create_problem_status_code}"
    publish_all_failure_logs
    echo "judge server flow create problem failed: expected status 201, got ${create_problem_status_code}" >&2
    echo "response body:" >&2
    cat "${create_problem_response_file}" >&2
    exit 1
fi

problem_id="$(
    python3 - "${create_problem_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = response.get("problem_id")
if not isinstance(problem_id, int) or problem_id <= 0:
    raise SystemExit("invalid problem_id in create problem response")

print(problem_id)
PY
)"

append_log_line "${test_log_temp_file}" "problem created: problem_id=${problem_id}"
print_success_log "problem create success"

limits_request_body="$(
    make_limits_request_body "${problem_memory_limit_mb}" "${problem_time_limit_ms}"
)"
set_limits_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${set_limits_response_file}" \
        --write-out "%{http_code}" \
        --request PUT \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${limits_request_body}" \
        "${base_url}/api/problem/${problem_id}/limits"
)"

if [[ "${set_limits_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "set limits failed: status=${set_limits_status_code}"
    publish_all_failure_logs
    echo "judge server flow set limits failed: expected status 200, got ${set_limits_status_code}" >&2
    echo "response body:" >&2
    cat "${set_limits_response_file}" >&2
    exit 1
fi

append_log_line "${test_log_temp_file}" "problem limits updated: problem_id=${problem_id}, memory_limit_mb=${problem_memory_limit_mb}, time_limit_ms=${problem_time_limit_ms}"
print_success_log "problem limits update success"

testcase_request_body="$(make_testcase_request_body)"
create_testcase_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${create_testcase_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${testcase_request_body}" \
        "${base_url}/api/problem/${problem_id}/testcases"
)"

if [[ "${create_testcase_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "create testcase failed: status=${create_testcase_status_code}"
    publish_all_failure_logs
    echo "judge server flow create testcase failed: expected status 201, got ${create_testcase_status_code}" >&2
    echo "response body:" >&2
    cat "${create_testcase_response_file}" >&2
    exit 1
fi

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
    curl \
        --silent \
        --show-error \
        --output "${submission_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${accepted_submission_request_body}" \
        "${base_url}/api/submission/${problem_id}"
)"

if [[ "${accepted_submission_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "accepted submission create failed: status=${accepted_submission_status_code}"
    publish_all_failure_logs
    echo "accepted submission create failed: expected status 201, got ${accepted_submission_status_code}" >&2
    echo "response body:" >&2
    cat "${submission_response_file}" >&2
    exit 1
fi

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
    "null"
validate_submission_status_history "${accepted_submission_id}" "accepted"
print_success_log "accepted submission judged successfully"

wrong_answer_submission_request_body="$(
    make_submission_request_body "cpp" "${wrong_answer_source_code}"
)"
wrong_answer_submission_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${submission_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${wrong_answer_submission_request_body}" \
        "${base_url}/api/submission/${problem_id}"
)"

if [[ "${wrong_answer_submission_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "wrong_answer submission create failed: status=${wrong_answer_submission_status_code}"
    publish_all_failure_logs
    echo "wrong_answer submission create failed: expected status 201, got ${wrong_answer_submission_status_code}" >&2
    echo "response body:" >&2
    cat "${submission_response_file}" >&2
    exit 1
fi

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
    "null"
validate_submission_status_history "${wrong_answer_submission_id}" "wrong_answer"
print_success_log "wrong_answer submission judged successfully"

compile_error_submission_request_body="$(
    make_submission_request_body "cpp" "${compile_error_source_code}"
)"
compile_error_submission_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${submission_response_file}" \
        --write-out "%{http_code}" \
        --request POST \
        -H "Authorization: Bearer ${sign_up_token}" \
        -H "Content-Type: application/json" \
        -d "${compile_error_submission_request_body}" \
        "${base_url}/api/submission/${problem_id}"
)"

if [[ "${compile_error_submission_status_code}" != "201" ]]; then
    append_log_line "${test_log_temp_file}" "compile_error submission create failed: status=${compile_error_submission_status_code}"
    publish_all_failure_logs
    echo "compile_error submission create failed: expected status 201, got ${compile_error_submission_status_code}" >&2
    echo "response body:" >&2
    cat "${submission_response_file}" >&2
    exit 1
fi

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
    "null"
validate_submission_status_history "${compile_error_submission_id}" "compile_error"
print_success_log "compile_error submission judged successfully"

problem_status_code="$(
    curl \
        --silent \
        --show-error \
        --output "${problem_response_file}" \
        --write-out "%{http_code}" \
        --request GET \
        "${base_url}/api/problem/${problem_id}"
)"

if [[ "${problem_status_code}" != "200" ]]; then
    append_log_line "${test_log_temp_file}" "problem statistics get failed: status=${problem_status_code}"
    publish_all_failure_logs
    echo "judge server flow problem get failed: expected status 200, got ${problem_status_code}" >&2
    echo "response body:" >&2
    cat "${problem_response_file}" >&2
    exit 1
fi

python3 - "${problem_response_file}" "${problem_id}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = int(sys.argv[2])
if response.get("problem_id") != problem_id:
    raise SystemExit("problem_id mismatch in problem detail response")

statistics = response.get("statistics")
if statistics != {"submission_count": 3, "accepted_count": 1}:
    raise SystemExit("problem statistics mismatch after judge flow")
PY

print_success_log "problem statistics validated"
append_log_line "${test_log_temp_file}" "judge server flow test passed"
print_success_log "judge server flow test passed: problem_id=${problem_id}, accepted_submission_id=${accepted_submission_id}, wrong_answer_submission_id=${wrong_answer_submission_id}, compile_error_submission_id=${compile_error_submission_id}"
