#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"
env_file="${project_root}/.env"

if [[ -f "${env_file}" ]]; then
    set -a
    # shellcheck disable=SC1090
    source "${env_file}"
    set +a
fi

if ! command -v psql >/dev/null 2>&1; then
    echo "error: psql command not found" >&2
    exit 1
fi

database_url="${DATABASE_URL:-}"
if [[ -z "${database_url}" ]]; then
    db_user="${DB_USER:-}"
    db_password="${DB_PASSWORD:-}"
    db_host="${DB_HOST:-}"
    db_port="${DB_PORT:-5432}"
    db_name="${DB_NAME:-}"

    if [[ -z "${db_user}" || -z "${db_password}" || -z "${db_host}" || -z "${db_name}" ]]; then
        echo "error: DATABASE_URL is empty" >&2
        echo "hint: set DATABASE_URL or DB_USER/DB_PASSWORD/DB_HOST/DB_PORT/DB_NAME in .env" >&2
        exit 1
    fi

    database_url="postgresql://${db_user}:${db_password}@${db_host}:${db_port}/${db_name}"
fi

diff_count="$(
    psql "${database_url}" \
        -X \
        -qAt \
        -v ON_ERROR_STOP=1 <<'SQL'
WITH expected AS (
    SELECT
        submission_table.user_id,
        submission_table.problem_id,
        COUNT(*)::BIGINT AS submission_count,
        COUNT(*) FILTER(
            WHERE submission_table.status = 'accepted'::submission_status
        )::BIGINT AS accepted_submission_count,
        COUNT(*) FILTER(
            WHERE submission_table.status IN (
                'wrong_answer'::submission_status,
                'time_limit_exceeded'::submission_status,
                'memory_limit_exceeded'::submission_status,
                'runtime_error'::submission_status,
                'compile_error'::submission_status,
                'output_exceeded'::submission_status
            )
        )::BIGINT AS failed_submission_count
    FROM submissions submission_table
    GROUP BY submission_table.user_id, submission_table.problem_id
), diff_rows AS (
    SELECT
        COALESCE(actual.user_id, expected.user_id) AS user_id,
        COALESCE(actual.problem_id, expected.problem_id) AS problem_id,
        actual.submission_count AS actual_submission_count,
        expected.submission_count AS expected_submission_count,
        actual.accepted_submission_count AS actual_accepted_submission_count,
        expected.accepted_submission_count AS expected_accepted_submission_count,
        actual.failed_submission_count AS actual_failed_submission_count,
        expected.failed_submission_count AS expected_failed_submission_count
    FROM user_problem_attempt_summary actual
    FULL OUTER JOIN expected
      ON actual.user_id = expected.user_id
     AND actual.problem_id = expected.problem_id
    WHERE
        actual.user_id IS NULL OR
        expected.user_id IS NULL OR
        actual.submission_count <> expected.submission_count OR
        actual.accepted_submission_count <> expected.accepted_submission_count OR
        actual.failed_submission_count <> expected.failed_submission_count
)
SELECT COUNT(*)::TEXT
FROM diff_rows;
SQL
)"

if [[ "${diff_count}" != "0" ]]; then
    echo "user_problem_attempt_summary validation failed" >&2
    psql "${database_url}" \
        -X \
        -v ON_ERROR_STOP=1 <<'SQL'
WITH expected AS (
    SELECT
        submission_table.user_id,
        submission_table.problem_id,
        COUNT(*)::BIGINT AS submission_count,
        COUNT(*) FILTER(
            WHERE submission_table.status = 'accepted'::submission_status
        )::BIGINT AS accepted_submission_count,
        COUNT(*) FILTER(
            WHERE submission_table.status IN (
                'wrong_answer'::submission_status,
                'time_limit_exceeded'::submission_status,
                'memory_limit_exceeded'::submission_status,
                'runtime_error'::submission_status,
                'compile_error'::submission_status,
                'output_exceeded'::submission_status
            )
        )::BIGINT AS failed_submission_count
    FROM submissions submission_table
    GROUP BY submission_table.user_id, submission_table.problem_id
)
SELECT
    COALESCE(actual.user_id, expected.user_id) AS user_id,
    COALESCE(actual.problem_id, expected.problem_id) AS problem_id,
    actual.submission_count AS actual_submission_count,
    expected.submission_count AS expected_submission_count,
    actual.accepted_submission_count AS actual_accepted_submission_count,
    expected.accepted_submission_count AS expected_accepted_submission_count,
    actual.failed_submission_count AS actual_failed_submission_count,
    expected.failed_submission_count AS expected_failed_submission_count
FROM user_problem_attempt_summary actual
FULL OUTER JOIN expected
  ON actual.user_id = expected.user_id
 AND actual.problem_id = expected.problem_id
WHERE
    actual.user_id IS NULL OR
    expected.user_id IS NULL OR
    actual.submission_count <> expected.submission_count OR
    actual.accepted_submission_count <> expected.accepted_submission_count OR
    actual.failed_submission_count <> expected.failed_submission_count
ORDER BY user_id, problem_id;
SQL
    exit 1
fi

echo "user_problem_attempt_summary validation passed"
