#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/load_test_util.sh"

project_root="${load_test_project_root}"

load_backend_env

http_port="${HTTP_SERVER_LOAD_TEST_HTTP_PORT:-18080}"
base_url="${HTTP_SERVER_LOAD_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${HTTP_SERVER_LOAD_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
request_count="${HTTP_SERVER_LOAD_TEST_REQUEST_COUNT:-600}"
concurrency="${HTTP_SERVER_LOAD_TEST_CONCURRENCY:-24}"
preload_submission_count="${HTTP_SERVER_LOAD_TEST_PRELOAD_SUBMISSION_COUNT:-24}"
problem_memory_limit_mb="${HTTP_SERVER_LOAD_TEST_MEMORY_LIMIT_MB:-256}"
problem_time_limit_ms="${HTTP_SERVER_LOAD_TEST_TIME_LIMIT_MS:-1000}"
user_login_id="${HTTP_SERVER_LOAD_TEST_LOGIN_ID:-$(make_test_login_id hl)}"
raw_password="${HTTP_SERVER_LOAD_TEST_PASSWORD:-password123}"
problem_title="${HTTP_SERVER_LOAD_TEST_PROBLEM_TITLE:-http load fixture}"
test_log_name="test_http_server_load.log"
server_log_name="test_http_server_load_http_server.log"
server_pid=""
test_log_path=""
server_log_path=""
test_database_created="0"

declare -a submission_ids=()
declare -a worker_metrics_files=()

cleanup(){
    local exit_status=$?
    trap - EXIT

    finish_flow_test cleanup_http_server teardown_isolated_test_database || exit_status=1
    exit "${exit_status}"
}

append_summary_to_log(){
    local summary_text="$1"

    while IFS= read -r summary_line; do
        [[ -n "${summary_line}" ]] || continue
        append_log_line "${test_log_temp_file}" "${summary_line}"
    done <<< "${summary_text}"
}

setup_http_fixture(){
    local limits_request_body=""
    local statement_request_body=""
    local testcase_request_body=""
    local preload_submission_request_body=""
    local problem_id_value=""
    local preload_index=0
    local response_status_code=""

    read -r sign_up_user_id sign_up_token < <(
        sign_up_user \
            "${user_login_id}" \
            "${raw_password}" \
            "${sign_up_response_file}" \
            "http server load"
    )

    promote_admin_user "${sign_up_user_id}" "http server load" >/dev/null

    problem_id_value="$(
        create_problem_via_api \
            "${sign_up_token}" \
            "${create_problem_response_file}" \
            "http server load" \
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
        "http server load set limits" \
        "${sign_up_token}" \
        "${limits_request_body}"

    statement_request_body="$(
        make_statement_request_body \
            "Return A+B." \
            "Two integers A and B are given." \
            "Print their sum." \
            "http server load fixture"
    )"
    send_http_request_and_assert_status \
        "PUT" \
        "${base_url}/api/problem/${problem_id_value}/statement" \
        "${set_statement_response_file}" \
        "200" \
        "http server load set statement" \
        "${sign_up_token}" \
        "${statement_request_body}"

    send_http_request_and_assert_status \
        "POST" \
        "${base_url}/api/problem/${problem_id_value}/sample" \
        "${create_sample_response_file}" \
        "201" \
        "http server load create sample" \
        "${sign_up_token}"

    testcase_request_body="$(make_testcase_request_body "1 2\n" "3\n")"
    send_http_request_and_assert_status \
        "POST" \
        "${base_url}/api/problem/${problem_id_value}/testcase" \
        "${create_testcase_response_file}" \
        "201" \
        "http server load create testcase" \
        "${sign_up_token}" \
        "${testcase_request_body}"

    preload_submission_request_body="$(
        make_submission_request_body \
            "cpp" \
            "#include <iostream>
int main(){
    std::cout << 3 << '\n';
    return 0;
}
"
    )"

    submission_ids=()
    for (( preload_index = 0; preload_index < preload_submission_count; ++preload_index )); do
        response_status_code="$(
            send_http_request \
                "POST" \
                "${base_url}/api/submission/${problem_id_value}" \
                "${create_submission_response_file}" \
                "${sign_up_token}" \
                "${preload_submission_request_body}"
        )"
        assert_status_code \
            "${response_status_code}" \
            "201" \
            "${create_submission_response_file}" \
            "http server load create preload submission"

        submission_ids+=(
            "$(read_submission_id_from_response "${create_submission_response_file}")"
        )
    done

    append_log_line \
        "${test_log_temp_file}" \
        "http fixture ready: user_id=${sign_up_user_id}, problem_id=${problem_id_value}, preload_submission_count=${#submission_ids[@]}"

    HTTP_SERVER_LOAD_SIGN_UP_USER_ID="${sign_up_user_id}"
    HTTP_SERVER_LOAD_SIGN_UP_TOKEN="${sign_up_token}"
    HTTP_SERVER_LOAD_PROBLEM_ID="${problem_id_value}"
    export \
        HTTP_SERVER_LOAD_SIGN_UP_USER_ID \
        HTTP_SERVER_LOAD_SIGN_UP_TOKEN \
        HTTP_SERVER_LOAD_PROBLEM_ID
}

run_http_load_worker(){
    local worker_index="$1"
    local worker_request_count="$2"
    local worker_metrics_file="$3"
    local response_file_path=""
    local request_index=0
    local scenario_index=0
    local scenario_name=""
    local expected_status_code=""
    local request_method=""
    local request_url=""
    local request_auth_token=""
    local request_body=""
    local status_code=""
    local start_ms=0
    local end_ms=0
    local duration_ms=0
    local submission_index=0

    response_file_path="$(mktemp)"
    trap 'rm -f "${response_file_path}"' RETURN

    for (( request_index = 0; request_index < worker_request_count; ++request_index )); do
        scenario_index=$(( (worker_index + request_index) % 6 ))
        request_auth_token=""
        request_body=""

        case "${scenario_index}" in
            0)
                scenario_name="health"
                expected_status_code="200"
                request_method="GET"
                request_url="${base_url}/api/system/health"
                ;;
            1)
                scenario_name="supported_languages"
                expected_status_code="200"
                request_method="GET"
                request_url="${base_url}/api/system/supported-languages"
                ;;
            2)
                scenario_name="problem_list"
                expected_status_code="200"
                request_method="GET"
                request_url="${base_url}/api/problem"
                ;;
            3)
                scenario_name="problem_detail"
                expected_status_code="200"
                request_method="GET"
                request_url="${base_url}/api/problem/${HTTP_SERVER_LOAD_PROBLEM_ID}"
                ;;
            4)
                scenario_name="auth_login"
                expected_status_code="200"
                request_method="POST"
                request_url="${base_url}/api/auth/login"
                request_body="${http_login_request_body}"
                ;;
            5)
                scenario_name="submission_detail"
                expected_status_code="200"
                request_method="GET"
                submission_index=$(( (worker_index + request_index) % ${#submission_ids[@]} ))
                request_url="${base_url}/api/submission/${submission_ids[submission_index]}"
                ;;
        esac

        start_ms="$(now_ms)"
        if status_code="$(
            send_http_request \
                "${request_method}" \
                "${request_url}" \
                "${response_file_path}" \
                "${request_auth_token}" \
                "${request_body}" \
                2>>"${test_log_temp_file}"
        )"; then
            :
        else
            status_code="curl_error"
        fi
        end_ms="$(now_ms)"
        duration_ms=$((end_ms - start_ms))

        if [[ "${status_code}" == "${expected_status_code}" ]]; then
            record_metric \
                "${worker_metrics_file}" \
                "${scenario_name}" \
                "1" \
                "${status_code}" \
                "${duration_ms}"
            continue
        fi

        append_log_line \
            "${test_log_temp_file}" \
            "http load request failed: worker=${worker_index}, scenario=${scenario_name}, status=${status_code}, expected=${expected_status_code}, duration_ms=${duration_ms}"

        record_metric \
            "${worker_metrics_file}" \
            "${scenario_name}" \
            "0" \
            "${status_code}" \
            "${duration_ms}"
    done
}

init_flow_test
trap cleanup EXIT

require_command curl
require_command python3
require_command psql
require_positive_integer "HTTP_SERVER_LOAD_TEST_HTTP_PORT" "${http_port}"
require_positive_integer "HTTP_SERVER_LOAD_TEST_REQUEST_COUNT" "${request_count}"
require_positive_integer "HTTP_SERVER_LOAD_TEST_CONCURRENCY" "${concurrency}"
require_positive_integer "HTTP_SERVER_LOAD_TEST_PRELOAD_SUBMISSION_COUNT" "${preload_submission_count}"
require_positive_integer "HTTP_SERVER_LOAD_TEST_MEMORY_LIMIT_MB" "${problem_memory_limit_mb}"
require_positive_integer "HTTP_SERVER_LOAD_TEST_TIME_LIMIT_MS" "${problem_time_limit_ms}"

register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file sign_up_response_file
register_temp_file create_problem_response_file
register_temp_file set_limits_response_file
register_temp_file set_statement_response_file
register_temp_file create_sample_response_file
register_temp_file create_testcase_response_file
register_temp_file create_submission_response_file
register_temp_file metrics_temp_file

export HTTP_WORKER_COUNT="${HTTP_SERVER_LOAD_TEST_HTTP_WORKER_COUNT:-${HTTP_WORKER_COUNT:-}}"
export HTTP_HANDLER_WORKER_COUNT="${HTTP_SERVER_LOAD_TEST_HTTP_HANDLER_WORKER_COUNT:-${HTTP_HANDLER_WORKER_COUNT:-}}"
export HTTP_DB_POOL_SIZE="${HTTP_SERVER_LOAD_TEST_HTTP_DB_POOL_SIZE:-${HTTP_DB_POOL_SIZE:-}}"

setup_isolated_test_database "http_server_load_test"
ensure_dedicated_http_server
setup_http_fixture

http_login_request_body="$(
    make_login_request_body "${user_login_id}" "${raw_password}"
)"

load_start_ms="$(now_ms)"
declare -a worker_pids=()
declare -a active_worker_metrics_files=()
for (( worker_index = 0; worker_index < concurrency; ++worker_index )); do
    worker_request_count=$(( request_count / concurrency ))
    if (( worker_index < request_count % concurrency )); then
        worker_request_count=$((worker_request_count + 1))
    fi

    if (( worker_request_count == 0 )); then
        continue
    fi

    register_temp_file worker_metrics_file
    worker_metrics_files+=("${worker_metrics_file}")
    active_worker_metrics_files+=("${worker_metrics_file}")

    run_http_load_worker \
        "${worker_index}" \
        "${worker_request_count}" \
        "${worker_metrics_file}" &
    worker_pids+=("$!")
done

worker_wait_failed="0"
for worker_pid in "${worker_pids[@]}"; do
    if ! wait "${worker_pid}"; then
        worker_wait_failed="1"
    fi
done
load_end_ms="$(now_ms)"

for worker_metrics_file in "${active_worker_metrics_files[@]}"; do
    cat "${worker_metrics_file}" >> "${metrics_temp_file}"
done

summary_text="$(
    print_metric_summary \
        "${metrics_temp_file}" \
        "http_server_load" \
        "$((load_end_ms - load_start_ms))"
)"
printf '%s\n' "${summary_text}"
append_summary_to_log "${summary_text}"

metrics_log_path="$(
    publish_log_file "${metrics_temp_file}" "test_http_server_load_metrics.tsv"
)"
print_log_file_created "${metrics_log_path}"

failure_count="$(count_metric_failures "${metrics_temp_file}")"
if [[ "${worker_wait_failed}" != "0" || "${failure_count}" != "0" ]]; then
    append_log_line \
        "${test_log_temp_file}" \
        "http_server_load failed: worker_wait_failed=${worker_wait_failed}, failure_count=${failure_count}"
    publish_failure_logs
    exit 1
fi

test_log_path="$(
    publish_log_file "${test_log_temp_file}" "${test_log_name}"
)"
print_log_file_created "${test_log_path}"
