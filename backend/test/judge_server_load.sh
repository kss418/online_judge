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
poller_pid=""
test_log_name="test_judge_server_load.log"
server_log_name="test_judge_server_load_http_server.log"
judge_server_log_name="test_judge_server_load_judge_server.log"
test_log_path=""
server_log_path=""
judge_server_log_path=""
test_database_created="0"

declare -a worker_metrics_files=()
declare -a worker_submission_info_files=()

cleanup(){
    local exit_status=$?
    trap - EXIT

    if [[ -n "${poller_pid:-}" ]]; then
        kill "${poller_pid}" >/dev/null 2>&1 || true
        wait "${poller_pid}" >/dev/null 2>&1 || true
    fi

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
            printf '%s\t%s\t%s\n' "${submission_id}" "${start_ms}" "${end_ms}" >> "${worker_submission_info_file}"
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

parse_submission_list_entries(){
    local response_file_path="$1"

    if [[ -z "${response_file_path}" ]]; then
        echo "missing response_file_path" >&2
        return 1
    fi

    python3 - "${response_file_path}" <<'PY'
import datetime
import json
import sys

response_file_path = sys.argv[1]
local_tz = datetime.datetime.now().astimezone().tzinfo


def to_epoch_ms(value):
    if not isinstance(value, str):
        return 0

    text = value.strip()
    if not text:
        return 0

    if text.endswith("Z"):
        text = text[:-1] + "+00:00"

    try:
        dt = datetime.datetime.fromisoformat(text)
    except ValueError:
        return 0

    if dt.tzinfo is None:
        dt = dt.replace(tzinfo=local_tz)

    return int(dt.timestamp() * 1000)


with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

submissions = response.get("submissions")
if not isinstance(submissions, list):
    raise SystemExit("missing submissions array")

for submission in submissions:
    if not isinstance(submission, dict):
        continue

    submission_id = submission.get("submission_id")
    status = submission.get("status")
    if not isinstance(submission_id, int) or not isinstance(status, str) or not status:
        continue

    print(
        f"{submission_id}\t{status}\t"
        f"{to_epoch_ms(submission.get('created_at'))}\t"
        f"{to_epoch_ms(submission.get('updated_at'))}"
    )
PY
}

run_submission_poller(){
    local poller_metrics_file="$1"
    local submit_workers_done_file="$2"
    shift 2
    local -a submission_info_files=("$@")
    local poll_response_file_path=""
    local known_submission_count=0
    local parsed_submission_entries=""
    local list_request_start_ms=0
    local list_request_end_ms=0
    local list_request_duration_ms=0
    local poll_status_code=""
    local poll_deadline_ms=0
    local submission_list_url=""
    local submission_id=""
    local current_status=""
    local created_at_ms=0
    local updated_at_ms=0
    local submission_start_ms=0
    local submission_visible_total_duration_ms=0
    local submission_terminal_server_duration_ms=0
    local submission_server_lifecycle_duration_ms=0
    local submission_observation_lag_ms=0
    local metric_is_success="0"

    local -A known_submission_started_at_ms=()
    local -A pending_submission_started_at_ms=()

    poll_response_file_path="$(mktemp)"
    trap 'rm -f "${poll_response_file_path}"' RETURN

    poll_deadline_ms=$(( $(now_ms) + max_wait_ms ))
    submission_list_url="${base_url}/api/submission?user_id=${JUDGE_SERVER_LOAD_SIGN_UP_USER_ID}&problem_id=${JUDGE_SERVER_LOAD_PROBLEM_ID}&limit=${submission_count}"

    while true; do
        for submission_info_file in "${submission_info_files[@]}"; do
            [[ -f "${submission_info_file}" ]] || continue

            while IFS=$'\t' read -r submission_id submission_start_ms _; do
                [[ -n "${submission_id}" ]] || continue
                if [[ -n "${known_submission_started_at_ms["${submission_id}"]+x}" ]]; then
                    continue
                fi

                known_submission_started_at_ms["${submission_id}"]="${submission_start_ms}"
                pending_submission_started_at_ms["${submission_id}"]="${submission_start_ms}"
                known_submission_count=$((known_submission_count + 1))
            done < "${submission_info_file}"
        done

        if [[ -f "${submit_workers_done_file}" ]] && (( ${#pending_submission_started_at_ms[@]} == 0 )); then
            break
        fi

        if (( $(now_ms) > poll_deadline_ms )); then
            for submission_id in "${!pending_submission_started_at_ms[@]}"; do
                submission_visible_total_duration_ms=$((poll_deadline_ms - pending_submission_started_at_ms["${submission_id}"]))
                record_metric \
                    "${poller_metrics_file}" \
                    "submission_total" \
                    "0" \
                    "timeout" \
                    "${submission_visible_total_duration_ms}"
                append_log_line \
                    "${test_log_temp_file}" \
                    "submission timeout: submission_id=${submission_id}, waited_ms=${submission_visible_total_duration_ms}"
            done
            break
        fi

        if (( known_submission_count == 0 )); then
            sleep "${poll_interval_seconds}"
            continue
        fi

        list_request_start_ms="$(now_ms)"
        if poll_status_code="$(
            send_http_request \
                "GET" \
                "${submission_list_url}" \
                "${poll_response_file_path}" \
                "${JUDGE_SERVER_LOAD_SIGN_UP_TOKEN}" \
                "" \
                2>>"${test_log_temp_file}"
        )"; then
            :
        else
            poll_status_code="curl_error"
        fi
        list_request_end_ms="$(now_ms)"
        list_request_duration_ms=$((list_request_end_ms - list_request_start_ms))

        if [[ "${poll_status_code}" != "200" ]]; then
            append_log_line \
                "${test_log_temp_file}" \
                "submission list poll failed: status=${poll_status_code}, duration_ms=${list_request_duration_ms}"
            record_metric \
                "${poller_metrics_file}" \
                "submission_list_poll" \
                "0" \
                "${poll_status_code}" \
                "${list_request_duration_ms}"
            sleep "${poll_interval_seconds}"
            continue
        fi

        record_metric \
            "${poller_metrics_file}" \
            "submission_list_poll" \
            "1" \
            "200" \
            "${list_request_duration_ms}"

        if ! parsed_submission_entries="$(
            parse_submission_list_entries "${poll_response_file_path}"
        )"; then
            append_log_line \
                "${test_log_temp_file}" \
                "submission list parse failed"
            return 1
        fi

        while IFS=$'\t' read -r submission_id current_status created_at_ms updated_at_ms; do
            [[ -n "${submission_id}" ]] || continue
            if [[ -z "${pending_submission_started_at_ms["${submission_id}"]+x}" ]]; then
                continue
            fi

            if ! is_terminal_submission_status "${current_status}"; then
                continue
            fi

            submission_start_ms="${pending_submission_started_at_ms["${submission_id}"]}"
            submission_visible_total_duration_ms=$((list_request_end_ms - submission_start_ms))
            metric_is_success="0"
            if [[ "${current_status}" == "${expected_final_status}" ]]; then
                metric_is_success="1"
            else
                append_log_line \
                    "${test_log_temp_file}" \
                    "unexpected final submission status: submission_id=${submission_id}, status=${current_status}, expected=${expected_final_status}"
            fi

            record_metric \
                "${poller_metrics_file}" \
                "submission_total" \
                "${metric_is_success}" \
                "${current_status}" \
                "${submission_visible_total_duration_ms}"
            record_metric \
                "${poller_metrics_file}" \
                "submission_visible_total" \
                "1" \
                "${current_status}" \
                "${submission_visible_total_duration_ms}"

            if (( updated_at_ms > 0 )); then
                submission_terminal_server_duration_ms=$((updated_at_ms - submission_start_ms))
                if (( submission_terminal_server_duration_ms >= 0 )); then
                    record_metric \
                        "${poller_metrics_file}" \
                        "submission_terminal_server" \
                        "1" \
                        "${current_status}" \
                        "${submission_terminal_server_duration_ms}"
                else
                    append_log_line \
                        "${test_log_temp_file}" \
                        "submission terminal timestamp earlier than client start: submission_id=${submission_id}, updated_at_ms=${updated_at_ms}, submission_start_ms=${submission_start_ms}"
                fi

                submission_observation_lag_ms=$((list_request_end_ms - updated_at_ms))
                if (( submission_observation_lag_ms < 0 )); then
                    append_log_line \
                        "${test_log_temp_file}" \
                        "submission observation lag negative, clamping to zero: submission_id=${submission_id}, updated_at_ms=${updated_at_ms}, observed_at_ms=${list_request_end_ms}"
                    submission_observation_lag_ms=0
                fi
                record_metric \
                    "${poller_metrics_file}" \
                    "submission_observation_lag" \
                    "1" \
                    "${current_status}" \
                    "${submission_observation_lag_ms}"
            else
                append_log_line \
                    "${test_log_temp_file}" \
                    "submission missing updated_at timestamp: submission_id=${submission_id}, status=${current_status}"
            fi

            if (( created_at_ms > 0 && updated_at_ms > 0 )); then
                submission_server_lifecycle_duration_ms=$((updated_at_ms - created_at_ms))
                if (( submission_server_lifecycle_duration_ms >= 0 )); then
                    record_metric \
                        "${poller_metrics_file}" \
                        "submission_server_lifecycle" \
                        "1" \
                        "${current_status}" \
                        "${submission_server_lifecycle_duration_ms}"
                else
                    append_log_line \
                        "${test_log_temp_file}" \
                        "submission lifecycle timestamp order invalid: submission_id=${submission_id}, created_at_ms=${created_at_ms}, updated_at_ms=${updated_at_ms}"
                fi
            else
                append_log_line \
                    "${test_log_temp_file}" \
                    "submission missing lifecycle timestamps: submission_id=${submission_id}, created_at_ms=${created_at_ms}, updated_at_ms=${updated_at_ms}"
            fi

            unset "pending_submission_started_at_ms[${submission_id}]"
        done <<< "${parsed_submission_entries}"

        if [[ -f "${submit_workers_done_file}" ]] && (( ${#pending_submission_started_at_ms[@]} == 0 )); then
            break
        fi

        sleep "${poll_interval_seconds}"
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
declare -a worker_indices=()
declare -a worker_submission_counts=()
register_temp_file poller_metrics_file
register_temp_file submit_workers_done_file
rm -f "${submit_workers_done_file}"
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
    worker_indices+=("${worker_index}")
    worker_submission_counts+=("${worker_submission_count}")
    worker_metrics_files+=("${worker_metrics_file}")
    worker_submission_info_files+=("${worker_submission_info_file}")
done

run_submission_poller \
    "${poller_metrics_file}" \
    "${submit_workers_done_file}" \
    "${worker_submission_info_files[@]}" &
poller_pid="$!"

for worker_slot in "${!worker_indices[@]}"; do
    run_submission_worker \
        "${worker_indices[worker_slot]}" \
        "${worker_submission_counts[worker_slot]}" \
        "${worker_metrics_files[worker_slot]}" \
        "${worker_submission_info_files[worker_slot]}" &
    worker_pids+=("$!")
done

worker_wait_failed="0"
for worker_pid in "${worker_pids[@]}"; do
    if ! wait "${worker_pid}"; then
        worker_wait_failed="1"
    fi
done

touch "${submit_workers_done_file}"

poller_wait_failed="0"
if ! wait "${poller_pid}"; then
    poller_wait_failed="1"
fi
poller_pid=""

created_submission_count=0
for worker_submission_info_file in "${worker_submission_info_files[@]}"; do
    while IFS=$'\t' read -r submission_id _ _; do
        [[ -n "${submission_id}" ]] || continue
        created_submission_count=$((created_submission_count + 1))
    done < "${worker_submission_info_file}"
done

for worker_metrics_file in "${worker_metrics_files[@]}"; do
    cat "${worker_metrics_file}" >> "${metrics_temp_file}"
done

cat "${poller_metrics_file}" >> "${metrics_temp_file}"

if (( created_submission_count == 0 )); then
    append_log_line "${test_log_temp_file}" "judge load created no submissions"
    publish_extended_failure_logs
    exit 1
fi
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
    publish_metric_file "${metrics_temp_file}" "test_judge_server_load_metrics.tsv"
)"
print_metric_file_created "${metrics_log_path}"
publish_judge_server_failure_log

failure_count="$(count_metric_failures "${metrics_temp_file}")"
if [[ "${worker_wait_failed}" != "0" || "${poller_wait_failed}" != "0" || "${failure_count}" != "0" ]]; then
    append_log_line \
        "${test_log_temp_file}" \
        "judge_server_load failed: worker_wait_failed=${worker_wait_failed}, poller_wait_failed=${poller_wait_failed}, failure_count=${failure_count}"
    publish_extended_failure_logs
    exit 1
fi

test_log_path="$(
    publish_log_file "${test_log_temp_file}" "${test_log_name}"
)"
print_log_file_created "${test_log_path}"
