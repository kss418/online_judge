#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/load_test_util.sh"

project_root="${load_test_project_root}"

load_backend_env

request_count="${DB_SERVER_LOAD_TEST_REQUEST_COUNT:-${DB_SERVER_LOAD_TEST_TRANSACTION_COUNT:-800}}"
concurrency="${DB_SERVER_LOAD_TEST_CONCURRENCY:-8}"
seed_user_count="${DB_SERVER_LOAD_TEST_SEED_USER_COUNT:-120}"
seed_problem_count="${DB_SERVER_LOAD_TEST_SEED_PROBLEM_COUNT:-60}"
seed_submission_count="${DB_SERVER_LOAD_TEST_SEED_SUBMISSION_COUNT:-5000}"
auth_user_count="${DB_SERVER_LOAD_TEST_AUTH_USER_COUNT:-${concurrency}}"
http_port="${DB_SERVER_LOAD_TEST_HTTP_PORT:-18081}"
base_url="${DB_SERVER_LOAD_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${DB_SERVER_LOAD_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
test_log_name="test_db_server_load.log"
server_log_name="test_db_server_load_http_server.log"
server_pid=""
test_log_path=""
server_log_path=""
test_database_created="0"
db_load_submission_request_body=""

declare -a auth_user_ids=()
declare -a auth_tokens=()

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

publish_db_failure_logs(){
    if [[ -n "${metrics_temp_file:-}" && -s "${metrics_temp_file}" ]]; then
        metrics_log_path="$(
            publish_metric_file "${metrics_temp_file}" "test_db_server_load_metrics.tsv"
        )"
        print_metric_file_created "${metrics_log_path}"
    fi

    if [[ "$(type -t publish_failure_logs || true)" == "function" ]]; then
        publish_failure_logs
        return 0
    fi

    if [[ -n "${test_log_temp_file:-}" && -s "${test_log_temp_file}" ]]; then
        test_log_path="$(
            publish_log_file "${test_log_temp_file}" "${test_log_name}"
        )"
        print_log_file_created "${test_log_path}"
    fi
}

seed_db_fixture(){
    append_log_line \
        "${test_log_temp_file}" \
        "seeding db fixture: users=${seed_user_count}, problems=${seed_problem_count}, submissions=${seed_submission_count}"

    PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v ON_ERROR_STOP=1 \
        -v seed_user_count="${seed_user_count}" \
        -v seed_problem_count="${seed_problem_count}" \
        -v seed_submission_count="${seed_submission_count}" \
        >/dev/null <<'SQL'
BEGIN;

INSERT INTO user_info(user_id)
SELECT generated.user_id
FROM generate_series(1, :seed_user_count) AS generated(user_id);

SELECT setval(
    pg_get_serial_sequence('user_info', 'user_id'),
    :seed_user_count,
    true
);

INSERT INTO users(
    user_id,
    user_login_id,
    user_password_hash,
    permission_level,
    auth_updated_at
)
SELECT
    generated.user_id,
    'dbu' || lpad(generated.user_id::TEXT, 6, '0'),
    'seed-hash-' || generated.user_id,
    CASE WHEN generated.user_id = 1 THEN 1 ELSE 0 END,
    NOW()
FROM generate_series(1, :seed_user_count) AS generated(user_id);

INSERT INTO problems(problem_id, title, version)
SELECT
    generated.problem_id,
    'db problem ' || generated.problem_id,
    1
FROM generate_series(1, :seed_problem_count) AS generated(problem_id);

SELECT setval(
    pg_get_serial_sequence('problems', 'problem_id'),
    :seed_problem_count,
    true
);

INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at)
SELECT
    generated.problem_id,
    256,
    1000,
    NOW()
FROM generate_series(1, :seed_problem_count) AS generated(problem_id);

INSERT INTO problem_statistics(problem_id, submission_count, accepted_count, updated_at)
SELECT
    generated.problem_id,
    0,
    0,
    NOW()
FROM generate_series(1, :seed_problem_count) AS generated(problem_id);

INSERT INTO problem_statements(
    problem_id,
    description,
    input_format,
    output_format,
    sample_count,
    testcase_count,
    note,
    created_at,
    updated_at
)
SELECT
    generated.problem_id,
    'seed description',
    'seed input',
    'seed output',
    0,
    1,
    'db load fixture',
    NOW(),
    NOW()
FROM generate_series(1, :seed_problem_count) AS generated(problem_id);

INSERT INTO problem_testcases(
    problem_id,
    testcase_order,
    testcase_input,
    testcase_output,
    created_at
)
SELECT
    generated.problem_id,
    1,
    '1 2',
    '3',
    NOW()
FROM generate_series(1, :seed_problem_count) AS generated(problem_id);

WITH submission_seed AS (
    SELECT
        generated.submission_index,
        1 + ((generated.submission_index - 1) % :seed_user_count) AS user_id,
        1 + ((generated.submission_index - 1) % :seed_problem_count) AS problem_id,
        CASE generated.submission_index % 3
            WHEN 0 THEN 'cpp'
            WHEN 1 THEN 'python'
            ELSE 'java'
        END AS language,
        CASE generated.submission_index % 9
            WHEN 0 THEN 'accepted'
            WHEN 1 THEN 'wrong_answer'
            WHEN 2 THEN 'compile_error'
            WHEN 3 THEN 'runtime_error'
            WHEN 4 THEN 'time_limit_exceeded'
            WHEN 5 THEN 'memory_limit_exceeded'
            WHEN 6 THEN 'output_exceeded'
            WHEN 7 THEN 'judging'
            ELSE 'queued'
        END AS status_text,
        NOW() - make_interval(secs => (:seed_submission_count - generated.submission_index)) AS created_timestamp
    FROM generate_series(1, :seed_submission_count) AS generated(submission_index)
), inserted_submissions AS (
    INSERT INTO submissions(
        user_id,
        problem_id,
        problem_version,
        language,
        source_code,
        status,
        score,
        compile_output,
        judge_output,
        elapsed_ms,
        max_rss_kb,
        created_at,
        updated_at
    )
    SELECT
        submission_seed.user_id,
        submission_seed.problem_id,
        problems.version,
        submission_seed.language,
        'seed source ' || submission_seed.submission_index,
        submission_seed.status_text::submission_status,
        CASE
            WHEN submission_seed.status_text = 'accepted' THEN 100
            WHEN submission_seed.status_text IN ('queued', 'judging') THEN NULL
            ELSE 0
        END,
        CASE
            WHEN submission_seed.status_text = 'compile_error'
                THEN 'seed compile output'
            ELSE NULL
        END,
        CASE
            WHEN submission_seed.status_text IN (
                'runtime_error',
                'time_limit_exceeded',
                'memory_limit_exceeded',
                'output_exceeded'
            ) THEN 'seed judge output'
            ELSE NULL
        END,
        CASE
            WHEN submission_seed.status_text IN (
                'accepted',
                'wrong_answer',
                'time_limit_exceeded',
                'memory_limit_exceeded',
                'output_exceeded'
            ) THEN 10 + (submission_seed.submission_index % 40)
            ELSE NULL
        END,
        CASE
            WHEN submission_seed.status_text IN (
                'accepted',
                'wrong_answer',
                'time_limit_exceeded',
                'memory_limit_exceeded',
                'output_exceeded'
            ) THEN 1024 + (submission_seed.submission_index % 4096)
            ELSE NULL
        END,
        submission_seed.created_timestamp,
        submission_seed.created_timestamp
    FROM submission_seed
    JOIN problems
    ON problems.problem_id = submission_seed.problem_id
    RETURNING submission_id, status, created_at
)
INSERT INTO submission_status_history(
    submission_id,
    from_status,
    to_status,
    reason,
    created_at
)
SELECT
    inserted_submissions.submission_id,
    NULL,
    inserted_submissions.status,
    NULL,
    inserted_submissions.created_at
FROM inserted_submissions;

INSERT INTO submission_queue(
    submission_id,
    priority,
    attempt_no,
    lease_token,
    available_at,
    leased_until,
    created_at
)
SELECT
    submissions.submission_id,
    0,
    0,
    NULL,
    submissions.created_at,
    NULL,
    submissions.created_at
FROM submissions
WHERE submissions.status = 'queued'::submission_status;

UPDATE problem_statistics
SET
    submission_count = aggregated.submission_count,
    accepted_count = aggregated.accepted_count,
    updated_at = NOW()
FROM (
    SELECT
        submissions.problem_id,
        COUNT(*)::BIGINT AS submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'accepted'::submission_status
        )::BIGINT AS accepted_count
    FROM submissions
    GROUP BY submissions.problem_id
) AS aggregated
WHERE problem_statistics.problem_id = aggregated.problem_id;

DELETE FROM user_problem_attempt_summary;

INSERT INTO user_problem_attempt_summary(
    user_id,
    problem_id,
    submission_count,
    accepted_submission_count,
    failed_submission_count,
    updated_at
)
SELECT
    submissions.user_id,
    submissions.problem_id,
    COUNT(*)::BIGINT,
    COUNT(*) FILTER(
        WHERE submissions.status = 'accepted'::submission_status
    )::BIGINT,
    COUNT(*) FILTER(
        WHERE submissions.status IN (
            'wrong_answer'::submission_status,
            'time_limit_exceeded'::submission_status,
            'memory_limit_exceeded'::submission_status,
            'runtime_error'::submission_status,
            'compile_error'::submission_status,
            'output_exceeded'::submission_status
        )
    )::BIGINT,
    NOW()
FROM submissions
GROUP BY submissions.user_id, submissions.problem_id;

UPDATE user_submission_statistics
SET
    submission_count = aggregated.submission_count,
    queued_submission_count = aggregated.queued_submission_count,
    judging_submission_count = aggregated.judging_submission_count,
    accepted_submission_count = aggregated.accepted_submission_count,
    wrong_answer_submission_count = aggregated.wrong_answer_submission_count,
    time_limit_exceeded_submission_count = aggregated.time_limit_exceeded_submission_count,
    memory_limit_exceeded_submission_count = aggregated.memory_limit_exceeded_submission_count,
    runtime_error_submission_count = aggregated.runtime_error_submission_count,
    compile_error_submission_count = aggregated.compile_error_submission_count,
    output_exceeded_submission_count = aggregated.output_exceeded_submission_count,
    infra_failure_submission_count = aggregated.infra_failure_submission_count,
    last_submission_at = aggregated.last_submission_at,
    last_accepted_at = aggregated.last_accepted_at,
    updated_at = NOW()
FROM (
    SELECT
        submissions.user_id,
        COUNT(*)::BIGINT AS submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'queued'::submission_status
        )::BIGINT AS queued_submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'judging'::submission_status
        )::BIGINT AS judging_submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'accepted'::submission_status
        )::BIGINT AS accepted_submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'wrong_answer'::submission_status
        )::BIGINT AS wrong_answer_submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'time_limit_exceeded'::submission_status
        )::BIGINT AS time_limit_exceeded_submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'memory_limit_exceeded'::submission_status
        )::BIGINT AS memory_limit_exceeded_submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'runtime_error'::submission_status
        )::BIGINT AS runtime_error_submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'compile_error'::submission_status
        )::BIGINT AS compile_error_submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'output_exceeded'::submission_status
        )::BIGINT AS output_exceeded_submission_count,
        COUNT(*) FILTER(
            WHERE submissions.status = 'infra_failure'::submission_status
        )::BIGINT AS infra_failure_submission_count,
        MAX(submissions.created_at) AS last_submission_at,
        MAX(submissions.created_at) FILTER(
            WHERE submissions.status = 'accepted'::submission_status
        ) AS last_accepted_at
    FROM submissions
    GROUP BY submissions.user_id
) AS aggregated
WHERE user_submission_statistics.user_id = aggregated.user_id;

COMMIT;
SQL
}

setup_http_fixture(){
    local auth_user_index=0
    local auth_user_login_id=""
    local auth_user_id=0
    local auth_token=""

    auth_user_ids=()
    auth_tokens=()

    for (( auth_user_index = 0; auth_user_index < auth_user_count; ++auth_user_index )); do
        auth_user_login_id="$(make_test_login_id dl "${auth_user_index}")"
        read -r auth_user_id auth_token < <(
            sign_up_user \
                "${auth_user_login_id}" \
                "password123" \
                "${sign_up_response_file}" \
                "db server load auth fixture"
        )

        auth_user_ids+=("${auth_user_id}")
        auth_tokens+=("${auth_token}")
    done

    db_load_submission_request_body="$(
        make_submission_request_body \
            "cpp" \
            "#include <iostream>
int main(){
    std::cout << 3 << '\\n';
    return 0;
}
"
    )"

    append_log_line \
        "${test_log_temp_file}" \
        "db http fixture ready: auth_user_count=${#auth_user_ids[@]}, seeded_problem_count=${seed_problem_count}, seeded_submission_count=${seed_submission_count}"
}

run_db_load_worker(){
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
    local seed_user_id=0
    local seed_problem_id=0
    local seed_submission_id=0
    local auth_user_index=0
    local auth_user_id=0
    local seeded_user_login_id=""

    response_file_path="$(mktemp)"
    trap 'rm -f "${response_file_path}"' RETURN

    for (( request_index = 0; request_index < worker_request_count; ++request_index )); do
        scenario_index=$(( (worker_index + request_index) % 7 ))
        request_auth_token=""
        request_body=""
        seed_user_id=$((1 + ((worker_index + request_index) % seed_user_count)))
        seed_problem_id=$((1 + ((worker_index + request_index) % seed_problem_count)))
        seed_submission_id=$((1 + ((worker_index + request_index) % seed_submission_count)))
        auth_user_index=$(( (worker_index + request_index) % ${#auth_tokens[@]} ))
        auth_user_id="${auth_user_ids[auth_user_index]}"
        printf -v seeded_user_login_id 'dbu%06d' "${seed_user_id}"

        case "${scenario_index}" in
            0)
                scenario_name="problem_list"
                expected_status_code="200"
                request_method="GET"
                request_url="${base_url}/api/problem"
                ;;
            1)
                scenario_name="problem_detail"
                expected_status_code="200"
                request_method="GET"
                request_url="${base_url}/api/problem/${seed_problem_id}"
                ;;
            2)
                scenario_name="submission_list"
                expected_status_code="200"
                request_method="GET"
                request_url="${base_url}/api/submission?user_id=${seed_user_id}&problem_id=${seed_problem_id}&limit=20"
                ;;
            3)
                scenario_name="submission_detail"
                expected_status_code="200"
                request_method="GET"
                request_url="${base_url}/api/submission/${seed_submission_id}"
                ;;
            4)
                scenario_name="public_user_list"
                expected_status_code="200"
                request_method="GET"
                request_url="${base_url}/api/user/list?q=${seeded_user_login_id}"
                ;;
            5)
                scenario_name="my_statistics"
                expected_status_code="200"
                request_method="GET"
                request_auth_token="${auth_tokens[auth_user_index]}"
                request_url="${base_url}/api/user/me/statistics"
                ;;
            6)
                scenario_name="submission_create"
                expected_status_code="201"
                request_method="POST"
                request_auth_token="${auth_tokens[auth_user_index]}"
                request_body="${db_load_submission_request_body}"
                request_url="${base_url}/api/submission/${seed_problem_id}"
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
            "db load request failed: worker=${worker_index}, scenario=${scenario_name}, status=${status_code}, expected=${expected_status_code}, duration_ms=${duration_ms}"

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
require_positive_integer "DB_SERVER_LOAD_TEST_REQUEST_COUNT" "${request_count}"
require_positive_integer "DB_SERVER_LOAD_TEST_CONCURRENCY" "${concurrency}"
require_positive_integer "DB_SERVER_LOAD_TEST_SEED_USER_COUNT" "${seed_user_count}"
require_positive_integer "DB_SERVER_LOAD_TEST_SEED_PROBLEM_COUNT" "${seed_problem_count}"
require_positive_integer "DB_SERVER_LOAD_TEST_SEED_SUBMISSION_COUNT" "${seed_submission_count}"
require_positive_integer "DB_SERVER_LOAD_TEST_AUTH_USER_COUNT" "${auth_user_count}"
require_positive_integer "DB_SERVER_LOAD_TEST_HTTP_PORT" "${http_port}"

register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file sign_up_response_file
register_temp_file metrics_temp_file

export HTTP_WORKER_COUNT="${DB_SERVER_LOAD_TEST_HTTP_WORKER_COUNT:-${HTTP_WORKER_COUNT:-}}"
export HTTP_HANDLER_WORKER_COUNT="${DB_SERVER_LOAD_TEST_HTTP_HANDLER_WORKER_COUNT:-${HTTP_HANDLER_WORKER_COUNT:-}}"
export HTTP_DB_POOL_SIZE="${DB_SERVER_LOAD_TEST_HTTP_DB_POOL_SIZE:-${HTTP_DB_POOL_SIZE:-}}"

setup_isolated_test_database "db_server_load_test"
seed_db_fixture
ensure_dedicated_http_server
setup_http_fixture

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
    active_worker_metrics_files+=("${worker_metrics_file}")

    run_db_load_worker \
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
        "db_server_load_workers" \
        "$((load_end_ms - load_start_ms))"
)"
printf '%s\n' "${summary_text}"
append_summary_to_log "${summary_text}"

db_throughput_summary="$(
    python3 - "${request_count}" "$((load_end_ms - load_start_ms))" <<'PY'
import sys

request_count = int(sys.argv[1])
elapsed_ms = int(sys.argv[2])
throughput_rps = 0.0
if elapsed_ms > 0:
    throughput_rps = request_count / (elapsed_ms / 1000.0)

print(
    f"db_server_load total_requests={request_count} "
    f"total_elapsed_ms={elapsed_ms} throughput_rps={throughput_rps:.2f}"
)
PY
)"
printf '%s\n' "${db_throughput_summary}"
append_log_line "${test_log_temp_file}" "${db_throughput_summary}"

metrics_log_path="$(
    publish_metric_file "${metrics_temp_file}" "test_db_server_load_metrics.tsv"
)"
print_metric_file_created "${metrics_log_path}"

failure_count="$(count_metric_failures "${metrics_temp_file}")"
if [[ "${worker_wait_failed}" != "0" || "${failure_count}" != "0" ]]; then
    append_log_line \
        "${test_log_temp_file}" \
        "db_server_load failed: worker_wait_failed=${worker_wait_failed}, failure_count=${failure_count}"
    publish_db_failure_logs
    exit 1
fi

test_log_path="$(
    publish_log_file "${test_log_temp_file}" "${test_log_name}"
)"
print_log_file_created "${test_log_path}"
