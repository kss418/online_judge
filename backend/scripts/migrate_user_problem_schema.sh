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

echo "apply user_problem_schema"
psql "${database_url}" \
    -v ON_ERROR_STOP=1 <<'SQL'
BEGIN;

CREATE TABLE IF NOT EXISTS schema_migrations(
    version TEXT PRIMARY KEY,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'users'
    ) THEN
        RAISE EXCEPTION 'auth_schema must be applied before user_problem_schema';
    END IF;

    IF NOT EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'problems'
    ) THEN
        RAISE EXCEPTION 'problem_schema must be applied before user_problem_schema';
    END IF;

    IF NOT EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'submissions'
    ) THEN
        RAISE EXCEPTION 'submission_schema must be applied before user_problem_schema';
    END IF;
END
$do$;

DROP VIEW IF EXISTS user_wrong_problem_list;
DROP VIEW IF EXISTS user_problem_list;
DROP VIEW IF EXISTS user_problem_attempt_summary;

CREATE VIEW user_problem_attempt_summary AS
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
GROUP BY submission_table.user_id, submission_table.problem_id;

CREATE VIEW user_wrong_problem_list AS
SELECT
    user_problem_attempt_summary.user_id,
    user_problem_attempt_summary.problem_id,
    user_problem_attempt_summary.submission_count,
    user_problem_attempt_summary.accepted_submission_count,
    user_problem_attempt_summary.failed_submission_count,
    'wrong'::TEXT AS problem_state
FROM user_problem_attempt_summary
WHERE user_problem_attempt_summary.accepted_submission_count = 0
  AND user_problem_attempt_summary.failed_submission_count > 0;

CREATE INDEX IF NOT EXISTS submissions_user_problem_idx
    ON submissions(user_id, problem_id);

INSERT INTO schema_migrations(version)
VALUES('user_problem_schema_v6')
ON CONFLICT(version) DO NOTHING;

COMMIT;
SQL

echo "migration completed"
