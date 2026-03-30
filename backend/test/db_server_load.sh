#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/load_test_util.sh"

project_root="${load_test_project_root}"

load_backend_env

transaction_count="${DB_SERVER_LOAD_TEST_TRANSACTION_COUNT:-800}"
concurrency="${DB_SERVER_LOAD_TEST_CONCURRENCY:-8}"
seed_user_count="${DB_SERVER_LOAD_TEST_SEED_USER_COUNT:-120}"
seed_problem_count="${DB_SERVER_LOAD_TEST_SEED_PROBLEM_COUNT:-60}"
seed_submission_count="${DB_SERVER_LOAD_TEST_SEED_SUBMISSION_COUNT:-5000}"
test_log_name="test_db_server_load.log"
test_log_path=""
test_database_created="0"

cleanup(){
    local exit_status=$?
    trap - EXIT

    finish_flow_test teardown_isolated_test_database || exit_status=1
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
            publish_log_file "${metrics_temp_file}" "test_db_server_load_metrics.tsv"
        )"
        print_log_file_created "${metrics_log_path}"
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
    attempt_count,
    available_at,
    leased_until,
    created_at
)
SELECT
    submissions.submission_id,
    0,
    0,
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

write_db_worker_sql_file(){
    local sql_file_path="$1"
    local iteration_count="$2"
    local iteration_index=0

    : > "${sql_file_path}"
    for (( iteration_index = 0; iteration_index < iteration_count; ++iteration_index )); do
        cat <<'SQL' >> "${sql_file_path}"
BEGIN;

SELECT problem_statistics.problem_id
FROM problem_statistics
WHERE problem_statistics.problem_id = 1 + floor(random() * :seed_problem_count)::BIGINT;

SELECT submissions.submission_id
FROM submissions
WHERE submissions.user_id = 1 + floor(random() * :seed_user_count)::BIGINT
ORDER BY submissions.created_at DESC
LIMIT 20;

SELECT user_problem_attempt_summary.user_id
FROM user_problem_attempt_summary
WHERE
    user_problem_attempt_summary.user_id = 1 + floor(random() * :seed_user_count)::BIGINT AND
    user_problem_attempt_summary.problem_id = 1 + floor(random() * :seed_problem_count)::BIGINT;

WITH picked_values AS (
    SELECT
        1 + floor(random() * :seed_user_count)::BIGINT AS user_id,
        1 + floor(random() * :seed_problem_count)::BIGINT AS problem_id
), inserted_submission AS (
    INSERT INTO submissions(
        user_id,
        problem_id,
        language,
        source_code,
        status,
        created_at,
        updated_at
    )
    SELECT
        picked_values.user_id,
        picked_values.problem_id,
        'cpp',
        'db load source',
        'queued'::submission_status,
        NOW(),
        NOW()
    FROM picked_values
    RETURNING submission_id
)
INSERT INTO submission_status_history(
    submission_id,
    from_status,
    to_status,
    reason
)
SELECT
    inserted_submission.submission_id,
    NULL,
    'queued'::submission_status,
    NULL
FROM inserted_submission;

INSERT INTO submission_queue(
    submission_id,
    priority,
    available_at,
    created_at
)
VALUES(
    currval(pg_get_serial_sequence('submissions', 'submission_id')),
    0,
    NOW(),
    NOW()
);

WITH submission_context AS (
    SELECT
        submissions.user_id,
        submissions.problem_id
    FROM submissions
    WHERE submissions.submission_id = currval(pg_get_serial_sequence('submissions', 'submission_id'))
)
UPDATE problem_statistics
SET
    submission_count = problem_statistics.submission_count + 1,
    updated_at = NOW()
WHERE problem_statistics.problem_id = (
    SELECT submission_context.problem_id
    FROM submission_context
);

WITH submission_context AS (
    SELECT
        submissions.user_id,
        submissions.problem_id
    FROM submissions
    WHERE submissions.submission_id = currval(pg_get_serial_sequence('submissions', 'submission_id'))
)
INSERT INTO user_problem_attempt_summary(
    user_id,
    problem_id,
    submission_count,
    accepted_submission_count,
    failed_submission_count,
    updated_at
)
SELECT
    submission_context.user_id,
    submission_context.problem_id,
    1,
    0,
    0,
    NOW()
FROM submission_context
ON CONFLICT(user_id, problem_id)
DO UPDATE SET
    submission_count = user_problem_attempt_summary.submission_count + 1,
    updated_at = NOW();

WITH submission_context AS (
    SELECT
        submissions.user_id
    FROM submissions
    WHERE submissions.submission_id = currval(pg_get_serial_sequence('submissions', 'submission_id'))
)
UPDATE user_submission_statistics
SET
    submission_count = user_submission_statistics.submission_count + 1,
    queued_submission_count = user_submission_statistics.queued_submission_count + 1,
    last_submission_at = NOW(),
    updated_at = NOW()
WHERE user_submission_statistics.user_id = (
    SELECT submission_context.user_id
    FROM submission_context
);

COMMIT;
SQL
    done
}

run_db_load_worker(){
    local worker_index="$1"
    local worker_transaction_count="$2"
    local worker_metrics_file="$3"
    local worker_sql_file="$4"
    local start_ms=0
    local end_ms=0
    local duration_ms=0

    write_db_worker_sql_file "${worker_sql_file}" "${worker_transaction_count}"

    start_ms="$(now_ms)"
    if PGPASSWORD="${DB_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_USER}" \
        -d "${DB_NAME}" \
        -v ON_ERROR_STOP=1 \
        -v seed_user_count="${seed_user_count}" \
        -v seed_problem_count="${seed_problem_count}" \
        -f "${worker_sql_file}" \
        >/dev/null \
        2>>"${test_log_temp_file}"; then
        end_ms="$(now_ms)"
        duration_ms=$((end_ms - start_ms))
        record_metric "${worker_metrics_file}" "db_worker" "1" "ok" "${duration_ms}"
        append_log_line \
            "${test_log_temp_file}" \
            "db worker complete: worker=${worker_index}, transaction_count=${worker_transaction_count}, duration_ms=${duration_ms}"
        return 0
    fi

    end_ms="$(now_ms)"
    duration_ms=$((end_ms - start_ms))
    record_metric "${worker_metrics_file}" "db_worker" "0" "psql_error" "${duration_ms}"
    append_log_line \
        "${test_log_temp_file}" \
        "db worker failed: worker=${worker_index}, transaction_count=${worker_transaction_count}, duration_ms=${duration_ms}"
    return 1
}

init_flow_test
trap cleanup EXIT

require_command python3
require_command psql
require_positive_integer "DB_SERVER_LOAD_TEST_TRANSACTION_COUNT" "${transaction_count}"
require_positive_integer "DB_SERVER_LOAD_TEST_CONCURRENCY" "${concurrency}"
require_positive_integer "DB_SERVER_LOAD_TEST_SEED_USER_COUNT" "${seed_user_count}"
require_positive_integer "DB_SERVER_LOAD_TEST_SEED_PROBLEM_COUNT" "${seed_problem_count}"
require_positive_integer "DB_SERVER_LOAD_TEST_SEED_SUBMISSION_COUNT" "${seed_submission_count}"

register_temp_file test_log_temp_file
register_temp_file metrics_temp_file

setup_isolated_test_database "db_server_load_test"
seed_db_fixture

load_start_ms="$(now_ms)"
declare -a worker_pids=()
declare -a active_worker_metrics_files=()
for (( worker_index = 0; worker_index < concurrency; ++worker_index )); do
    worker_transaction_count=$(( transaction_count / concurrency ))
    if (( worker_index < transaction_count % concurrency )); then
        worker_transaction_count=$((worker_transaction_count + 1))
    fi

    if (( worker_transaction_count == 0 )); then
        continue
    fi

    register_temp_file worker_metrics_file
    register_temp_file worker_sql_file
    active_worker_metrics_files+=("${worker_metrics_file}")

    run_db_load_worker \
        "${worker_index}" \
        "${worker_transaction_count}" \
        "${worker_metrics_file}" \
        "${worker_sql_file}" &
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
    python3 - "${transaction_count}" "$((load_end_ms - load_start_ms))" <<'PY'
import sys

transaction_count = int(sys.argv[1])
elapsed_ms = int(sys.argv[2])
throughput_tps = 0.0
if elapsed_ms > 0:
    throughput_tps = transaction_count / (elapsed_ms / 1000.0)

print(
    f"db_server_load total_transactions={transaction_count} "
    f"total_elapsed_ms={elapsed_ms} throughput_tps={throughput_tps:.2f}"
)
PY
)"
printf '%s\n' "${db_throughput_summary}"
append_log_line "${test_log_temp_file}" "${db_throughput_summary}"

metrics_log_path="$(
    publish_log_file "${metrics_temp_file}" "test_db_server_load_metrics.tsv"
)"
print_log_file_created "${metrics_log_path}"

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
