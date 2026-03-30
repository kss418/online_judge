#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/load_test_util.sh"

project_root="${load_test_project_root}"

load_backend_env

http_port="${JUDGE_SERVER_LOAD_TEST_HTTP_PORT:-18081}"
base_url="${JUDGE_SERVER_LOAD_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${JUDGE_SERVER_LOAD_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
judge_server_bin="${JUDGE_SERVER_LOAD_TEST_JUDGE_SERVER_BIN:-${project_root}/judge_server}"
submission_count="${JUDGE_SERVER_LOAD_TEST_SUBMISSION_COUNT:-48}"
submit_concurrency="${JUDGE_SERVER_LOAD_TEST_SUBMIT_CONCURRENCY:-12}"
poll_interval_ms="${JUDGE_SERVER_LOAD_TEST_POLL_INTERVAL_MS:-200}"
max_wait_ms="${JUDGE_SERVER_LOAD_TEST_MAX_WAIT_MS:-180000}"
problem_memory_limit_mb="${JUDGE_SERVER_LOAD_TEST_MEMORY_LIMIT_MB:-256}"
problem_time_limit_ms="${JUDGE_SERVER_LOAD_TEST_TIME_LIMIT_MS:-1000}"
submission_language="${JUDGE_SERVER_LOAD_TEST_LANGUAGE:-cpp}"
expected_final_status="${JUDGE_SERVER_LOAD_TEST_EXPECTED_FINAL_STATUS:-accepted}"
user_login_id="${JUDGE_SERVER_LOAD_TEST_LOGIN_ID:-$(make_test_login_id jl)}"
raw_password="${JUDGE_SERVER_LOAD_TEST_PASSWORD:-password123}"
problem_title="${JUDGE_SERVER_LOAD_TEST_PROBLEM_TITLE:-judge load fixture}"
server_pid=""
judge_server_pid=""
test_log_name="test_judge_server_load.log"
server_log_name="test_judge_server_load_http_server.log"
judge_server_log_name="test_judge_server_load_judge_server.log"
test_log_path=""
server_log_path=""
judge_server_log_path=""
test_database_created="0"

declare -a worker_metrics_files=()
declare -a worker_submission_info_files=()
declare -A submission_started_at_ms=()

cleanup(){
    local exit_status=$?
    trap - EXIT

    finish_flow_test \
        cleanup_judge_server \
        cleanup_http_server \
        teardown_isolated_test_database || exit_status=1
    exit "${exit_status}"
}

append_summary_to_log(){
    local summary_text="$1"

    while IFS= read -r summary_line; do
        [[ -n "${summary_line}" ]] || continue
        append_log_line "${test_log_temp_file}" "${summary_line}"
    done <<< "${summary_text}"
}

resolve_submission_source_code(){
    if [[ -n "${JUDGE_SERVER_LOAD_TEST_SOURCE_CODE:-}" ]]; then
        printf '%s\n' "${JUDGE_SERVER_LOAD_TEST_SOURCE_CODE}"
        return 0
    fi

    case "${submission_language}" in
        cpp)
            cat <<'EOF'
#include <iostream>
int main(){
    std::cout << 3 << '\n';
    return 0;
}
EOF
            ;;
        python)
            printf '%s\n' "print(3)"
            ;;
        java)
            cat <<'EOF'
public class Main {
    public static void main(String[] args){
        System.out.println(3);
    }
}
EOF
            ;;
        *)
            echo "unsupported JUDGE_SERVER_LOAD_TEST_LANGUAGE: ${submission_language}" >&2
            return 1
            ;;
    esac
}

setup_judge_fixture(){
    local problem_id_value=""
    local limits_request_body=""
    local testcase_request_body=""

    read -r sign_up_user_id sign_up_token < <(
        sign_up_user \
            "${user_login_id}" \
            "${raw_password}" \
            "${sign_up_response_file}" \
            "judge server load"
    )

    promote_admin_user "${sign_up_user_id}" "judge server load" >/dev/null

    problem_id_value="$(
        create_problem_via_api \
            "${sign_up_token}" \
            "${create_problem_response_file}" \
            "judge server load" \
            "${problem_title}"
    )"

    limits_request_body="$(
        make_limits_request_body \
            "${problem_memory_limit_mb}" \
            "${problem_time_limit_ms}"
    )"
    send_http_request_and_assert_status \
        "PUT" \
        "${base_url}/api/problem/${problem_id_value}/limits" \
        "${set_limits_response_file}" \
        "200" \
        "judge server load set limits" \
        "${sign_up_token}" \
        "${limits_request_body}"

    testcase_request_body="$(
        make_testcase_request_body             $'1 2
'             $'3
'
    )"
    send_http_request_and_assert_status \
        "POST" \
        "${base_url}/api/problem/${problem_id_value}/testcase" \
        "${create_testcase_response_file}" \
        "201" \
        "judge server load create testcase" \
        "${sign_up_token}" \
        "${testcase_request_body}"

    JUDGE_SERVER_LOAD_SIGN_UP_USER_ID="${sign_up_user_id}"
    JUDGE_SERVER_LOAD_SIGN_UP_TOKEN="${sign_up_token}"
    JUDGE_SERVER_LOAD_PROBLEM_ID="${problem_id_value}"
    export \
        JUDGE_SERVER_LOAD_SIGN_UP_USER_ID \
        JUDGE_SERVER_LOAD_SIGN_UP_TOKEN \
        JUDGE_SERVER_LOAD_PROBLEM_ID

    append_log_line \
        "${test_log_temp_file}" \
        "judge fixture ready: user_id=${sign_up_user_id}, problem_id=${problem_id_value}"
}

run_submission_worker(){
    local worker_index="$1"
    local worker_submission_count="$2"
    local worker_metrics_file="$3"
    local worker_submission_info_file="$4"
    local response_file_path=""
    local submission_index=0
    local status_code=""
    local start_ms=0
    local end_ms=0
    local duration_ms=0
    local submission_id=""

    response_file_path="$(mktemp)"
    trap 'rm -f "${response_file_path}"' RETURN

    for (( submission_index = 0; submission_index < worker_submission_count; ++submission_index )); do
        start_ms="$(now_ms)"
        if status_code="$(
            send_http_request \
                "POST" \
                "${base_url}/api/submission/${JUDGE_SERVER_LOAD_PROBLEM_ID}" \
                "${response_file_path}" \
                "${JUDGE_SERVER_LOAD_SIGN_UP_TOKEN}" \
                "${judge_submission_request_body}" \
                2>>"${test_log_temp_file}"
        )"; then
            :
        else
            status_code="curl_error"
        fi
        end_ms="$(now_ms)"
        duration_ms=$((end_ms - start_ms))

        if [[ "${status_code}" == "201" ]]; then
            submission_id="$(read_submission_id_from_response "${response_file_path}")"
            printf '%s\t%s\n' "${submission_id}" "${start_ms}" >> "${worker_submission_info_file}"
            record_metric \
                "${worker_metrics_file}" \
                "submission_create" \
                "1" \
                "${status_code}" \
                "${duration_ms}"
            continue
        fi

        append_log_line \
            "${test_log_temp_file}" \
            "submission create failed: worker=${worker_index}, status=${status_code}, duration_ms=${duration_ms}"

        record_metric \
            "${worker_metrics_file}" \
            "submission_create" \
            "0" \
            "${status_code}" \
            "${duration_ms}"
    done
}

is_terminal_submission_status(){
    case "$1" in
        queued|judging)
            return 1
            ;;
        *)
            return 0
            ;;
    esac
}

init_flow_test
trap cleanup EXIT

require_command curl
require_command python3
require_command psql
require_positive_integer "JUDGE_SERVER_LOAD_TEST_HTTP_PORT" "${http_port}"
require_positive_integer "JUDGE_SERVER_LOAD_TEST_SUBMISSION_COUNT" "${submission_count}"
require_positive_integer "JUDGE_SERVER_LOAD_TEST_SUBMIT_CONCURRENCY" "${submit_concurrency}"
require_positive_integer "JUDGE_SERVER_LOAD_TEST_POLL_INTERVAL_MS" "${poll_interval_ms}"
require_positive_integer "JUDGE_SERVER_LOAD_TEST_MAX_WAIT_MS" "${max_wait_ms}"
require_positive_integer "JUDGE_SERVER_LOAD_TEST_MEMORY_LIMIT_MB" "${problem_memory_limit_mb}"
require_positive_integer "JUDGE_SERVER_LOAD_TEST_TIME_LIMIT_MS" "${problem_time_limit_ms}"

register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file judge_server_log_temp_file
register_temp_file sign_up_response_file
register_temp_file create_problem_response_file
register_temp_file set_limits_response_file
register_temp_file create_testcase_response_file
register_temp_file metrics_temp_file
register_temp_file poll_response_file
register_temp_dir judge_source_root
register_temp_dir testcase_root

judge_submission_source_code="$(resolve_submission_source_code)"
judge_submission_request_body="$(
    make_submission_request_body \
        "${submission_language}" \
        "${judge_submission_source_code}"
)"
poll_interval_seconds="$(
    python3 - "${poll_interval_ms}" <<'PY'
import sys

print(f"{int(sys.argv[1]) / 1000:.3f}")
PY
)"

export HTTP_WORKER_COUNT="${JUDGE_SERVER_LOAD_TEST_HTTP_WORKER_COUNT:-${HTTP_WORKER_COUNT:-}}"
export HTTP_HANDLER_WORKER_COUNT="${JUDGE_SERVER_LOAD_TEST_HTTP_HANDLER_WORKER_COUNT:-${HTTP_HANDLER_WORKER_COUNT:-}}"
export HTTP_DB_POOL_SIZE="${JUDGE_SERVER_LOAD_TEST_HTTP_DB_POOL_SIZE:-${HTTP_DB_POOL_SIZE:-}}"
export WORKER_ID="${JUDGE_SERVER_LOAD_TEST_WORKER_ID:-judge-load-$$_1}"
export MAX_CONCURRENT_JOBS="${JUDGE_SERVER_LOAD_TEST_MAX_CONCURRENT_JOBS:-${MAX_CONCURRENT_JOBS:-2}}"
export JUDGE_WORKER_COUNT="${JUDGE_SERVER_LOAD_TEST_WORKER_COUNT:-${JUDGE_WORKER_COUNT:-}}"
export JUDGE_SOURCE_ROOT="${judge_source_root}"
export TESTCASE_PATH="${testcase_root}"

setup_isolated_test_database "judge_server_load_test"
ensure_dedicated_http_server
ensure_dedicated_judge_server
setup_judge_fixture

load_start_ms="$(now_ms)"
declare -a worker_pids=()
declare -a active_worker_metrics_files=()
declare -a active_worker_submission_info_files=()
for (( worker_index = 0; worker_index < submit_concurrency; ++worker_index )); do
    worker_submission_count=$(( submission_count / submit_concurrency ))
    if (( worker_index < submission_count % submit_concurrency )); then
        worker_submission_count=$((worker_submission_count + 1))
    fi

    if (( worker_submission_count == 0 )); then
        continue
    fi

    register_temp_file worker_metrics_file
    register_temp_file worker_submission_info_file
    worker_metrics_files+=("${worker_metrics_file}")
    worker_submission_info_files+=("${worker_submission_info_file}")
    active_worker_metrics_files+=("${worker_metrics_file}")
    active_worker_submission_info_files+=("${worker_submission_info_file}")

    run_submission_worker \
        "${worker_index}" \
        "${worker_submission_count}" \
        "${worker_metrics_file}" \
        "${worker_submission_info_file}" &
    worker_pids+=("$!")
done

worker_wait_failed="0"
for worker_pid in "${worker_pids[@]}"; do
    if ! wait "${worker_pid}"; then
        worker_wait_failed="1"
    fi
done

for worker_metrics_file in "${active_worker_metrics_files[@]}"; do
    cat "${worker_metrics_file}" >> "${metrics_temp_file}"
done

for worker_submission_info_file in "${active_worker_submission_info_files[@]}"; do
    while IFS=$'\t' read -r submission_id submission_start_ms; do
        [[ -n "${submission_id}" ]] || continue
        submission_started_at_ms["${submission_id}"]="${submission_start_ms}"
    done < "${worker_submission_info_file}"
done

if (( ${#submission_started_at_ms[@]} == 0 )); then
    append_log_line "${test_log_temp_file}" "judge load created no submissions"
    publish_extended_failure_logs
    exit 1
fi

poll_deadline_ms=$(( $(now_ms) + max_wait_ms ))
while (( ${#submission_started_at_ms[@]} > 0 )); do
    if (( $(now_ms) > poll_deadline_ms )); then
        for submission_id in "${!submission_started_at_ms[@]}"; do
            waited_ms=$((poll_deadline_ms - submission_started_at_ms["${submission_id}"]))
            record_metric \
                "${metrics_temp_file}" \
                "submission_total" \
                "0" \
                "timeout" \
                "${waited_ms}"
            append_log_line \
                "${test_log_temp_file}" \
                "submission timeout: submission_id=${submission_id}, waited_ms=${waited_ms}"
        done
        break
    fi

    for submission_id in "${!submission_started_at_ms[@]}"; do
        poll_request_start_ms="$(now_ms)"
        if poll_status_code="$(
            send_http_request \
                "GET" \
                "${base_url}/api/submission/${submission_id}" \
                "${poll_response_file}" \
                "" \
                "" \
                2>>"${test_log_temp_file}"
        )"; then
            :
        else
            poll_status_code="curl_error"
        fi
        poll_request_end_ms="$(now_ms)"
        poll_request_duration_ms=$((poll_request_end_ms - poll_request_start_ms))

        if [[ "${poll_status_code}" != "200" ]]; then
            append_log_line \
                "${test_log_temp_file}" \
                "submission poll failed: submission_id=${submission_id}, status=${poll_status_code}, duration_ms=${poll_request_duration_ms}"
            record_metric \
                "${metrics_temp_file}" \
                "submission_poll" \
                "0" \
                "${poll_status_code}" \
                "${poll_request_duration_ms}"
            continue
        fi

        current_status="$(read_json_field "${poll_response_file}" "status" "string")"
        record_metric \
            "${metrics_temp_file}" \
            "submission_poll" \
            "1" \
            "${current_status}" \
            "${poll_request_duration_ms}"

        if ! is_terminal_submission_status "${current_status}"; then
            continue
        fi

        submission_total_duration_ms=$((poll_request_end_ms - submission_started_at_ms["${submission_id}"]))
        if [[ "${current_status}" == "${expected_final_status}" ]]; then
            record_metric \
                "${metrics_temp_file}" \
                "submission_total" \
                "1" \
                "${current_status}" \
                "${submission_total_duration_ms}"
        else
            record_metric \
                "${metrics_temp_file}" \
                "submission_total" \
                "0" \
                "${current_status}" \
                "${submission_total_duration_ms}"
            append_log_line \
                "${test_log_temp_file}" \
                "unexpected final submission status: submission_id=${submission_id}, status=${current_status}, expected=${expected_final_status}"
        fi

        unset "submission_started_at_ms[${submission_id}]"
    done

    if (( ${#submission_started_at_ms[@]} > 0 )); then
        sleep "${poll_interval_seconds}"
    fi
done
load_end_ms="$(now_ms)"

summary_text="$(
    print_metric_summary \
        "${metrics_temp_file}" \
        "judge_server_load" \
        "$((load_end_ms - load_start_ms))"
)"
printf '%s\n' "${summary_text}"
append_summary_to_log "${summary_text}"

metrics_log_path="$(
    publish_log_file "${metrics_temp_file}" "test_judge_server_load_metrics.tsv"
)"
print_log_file_created "${metrics_log_path}"
publish_judge_server_failure_log

failure_count="$(count_metric_failures "${metrics_temp_file}")"
if [[ "${worker_wait_failed}" != "0" || "${failure_count}" != "0" ]]; then
    append_log_line \
        "${test_log_temp_file}" \
        "judge_server_load failed: worker_wait_failed=${worker_wait_failed}, failure_count=${failure_count}"
    publish_extended_failure_logs
    exit 1
fi

test_log_path="$(
    publish_log_file "${test_log_temp_file}" "${test_log_name}"
)"
print_log_file_created "${test_log_path}"
