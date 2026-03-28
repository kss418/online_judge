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

echo "apply user_statistics_schema"
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
        RAISE EXCEPTION 'auth_schema must be applied before user_statistics_schema';
    END IF;

    IF NOT EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'submissions'
    ) THEN
        RAISE EXCEPTION 'submission_schema must be applied before user_statistics_schema';
    END IF;
END
$do$;

CREATE TABLE IF NOT EXISTS user_submission_statistics(
    user_id BIGINT PRIMARY KEY REFERENCES users(user_id) ON DELETE CASCADE,
    submission_count BIGINT NOT NULL DEFAULT 0,
    queued_submission_count BIGINT NOT NULL DEFAULT 0,
    judging_submission_count BIGINT NOT NULL DEFAULT 0,
    accepted_submission_count BIGINT NOT NULL DEFAULT 0,
    wrong_answer_submission_count BIGINT NOT NULL DEFAULT 0,
    time_limit_exceeded_submission_count BIGINT NOT NULL DEFAULT 0,
    memory_limit_exceeded_submission_count BIGINT NOT NULL DEFAULT 0,
    runtime_error_submission_count BIGINT NOT NULL DEFAULT 0,
    compile_error_submission_count BIGINT NOT NULL DEFAULT 0,
    output_exceeded_submission_count BIGINT NOT NULL DEFAULT 0,
    last_submission_at TIMESTAMPTZ,
    last_accepted_at TIMESTAMPTZ,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT user_submission_statistics_non_negative_check CHECK(
        submission_count >= 0 AND
        queued_submission_count >= 0 AND
        judging_submission_count >= 0 AND
        accepted_submission_count >= 0 AND
        wrong_answer_submission_count >= 0 AND
        time_limit_exceeded_submission_count >= 0 AND
        memory_limit_exceeded_submission_count >= 0 AND
        runtime_error_submission_count >= 0 AND
        compile_error_submission_count >= 0 AND
        output_exceeded_submission_count >= 0
    ),
    CONSTRAINT user_submission_statistics_total_check CHECK(
        submission_count =
            queued_submission_count +
            judging_submission_count +
            accepted_submission_count +
            wrong_answer_submission_count +
            time_limit_exceeded_submission_count +
            memory_limit_exceeded_submission_count +
            runtime_error_submission_count +
            compile_error_submission_count +
            output_exceeded_submission_count
    )
);

CREATE OR REPLACE FUNCTION ensure_user_submission_statistics_row()
RETURNS TRIGGER
LANGUAGE plpgsql
AS $function$
BEGIN
    INSERT INTO user_submission_statistics(user_id)
    VALUES(NEW.user_id)
    ON CONFLICT(user_id) DO NOTHING;

    RETURN NEW;
END
$function$;

DROP TRIGGER IF EXISTS users_insert_user_submission_statistics ON users;

CREATE TRIGGER users_insert_user_submission_statistics
AFTER INSERT ON users
FOR EACH ROW
EXECUTE FUNCTION ensure_user_submission_statistics_row();

INSERT INTO user_submission_statistics(
    user_id,
    submission_count,
    queued_submission_count,
    judging_submission_count,
    accepted_submission_count,
    wrong_answer_submission_count,
    time_limit_exceeded_submission_count,
    memory_limit_exceeded_submission_count,
    runtime_error_submission_count,
    compile_error_submission_count,
    output_exceeded_submission_count,
    last_submission_at,
    last_accepted_at,
    updated_at
)
SELECT
    user_table.user_id,
    COUNT(submission_table.submission_id)::BIGINT AS submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'queued'::submission_status
    )::BIGINT AS queued_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'judging'::submission_status
    )::BIGINT AS judging_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'accepted'::submission_status
    )::BIGINT AS accepted_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'wrong_answer'::submission_status
    )::BIGINT AS wrong_answer_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'time_limit_exceeded'::submission_status
    )::BIGINT AS time_limit_exceeded_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'memory_limit_exceeded'::submission_status
    )::BIGINT AS memory_limit_exceeded_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'runtime_error'::submission_status
    )::BIGINT AS runtime_error_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'compile_error'::submission_status
    )::BIGINT AS compile_error_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'output_exceeded'::submission_status
    )::BIGINT AS output_exceeded_submission_count,
    MAX(submission_table.created_at) AS last_submission_at,
    MAX(submission_table.created_at) FILTER(
        WHERE submission_table.status = 'accepted'::submission_status
    ) AS last_accepted_at,
    NOW() AS updated_at
FROM users user_table
LEFT JOIN submissions submission_table
    ON submission_table.user_id = user_table.user_id
GROUP BY user_table.user_id
ON CONFLICT(user_id) DO UPDATE
SET
    submission_count = EXCLUDED.submission_count,
    queued_submission_count = EXCLUDED.queued_submission_count,
    judging_submission_count = EXCLUDED.judging_submission_count,
    accepted_submission_count = EXCLUDED.accepted_submission_count,
    wrong_answer_submission_count = EXCLUDED.wrong_answer_submission_count,
    time_limit_exceeded_submission_count = EXCLUDED.time_limit_exceeded_submission_count,
    memory_limit_exceeded_submission_count = EXCLUDED.memory_limit_exceeded_submission_count,
    runtime_error_submission_count = EXCLUDED.runtime_error_submission_count,
    compile_error_submission_count = EXCLUDED.compile_error_submission_count,
    output_exceeded_submission_count = EXCLUDED.output_exceeded_submission_count,
    last_submission_at = EXCLUDED.last_submission_at,
    last_accepted_at = EXCLUDED.last_accepted_at,
    updated_at = EXCLUDED.updated_at;

INSERT INTO schema_migrations(version)
VALUES('user_statistics_schema_v1')
ON CONFLICT(version) DO NOTHING;

COMMIT;
SQL

echo "migration completed"
