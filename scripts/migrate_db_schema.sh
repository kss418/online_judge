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

echo "apply submission_history"
psql "${database_url}" \
    -v ON_ERROR_STOP=1 <<'SQL'
BEGIN;

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM pg_type
        WHERE typname = 'submission_status'
    ) THEN
        CREATE TYPE submission_status AS ENUM(
            'queued',
            'judging',
            'accepted',
            'wrong_answer',
            'time_limit_exceeded',
            'memory_limit_exceeded',
            'runtime_error',
            'compile_error',
            'output_exceeded'
        );
    END IF;
END
$do$;

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM pg_type type_table
        JOIN pg_enum enum_table
            ON enum_table.enumtypid = type_table.oid
        WHERE
            type_table.typname = 'submission_status' AND
            enum_table.enumlabel = 'system_error'
    ) AND NOT EXISTS(
        SELECT 1
        FROM pg_type type_table
        JOIN pg_enum enum_table
            ON enum_table.enumtypid = type_table.oid
        WHERE
            type_table.typname = 'submission_status' AND
            enum_table.enumlabel = 'output_exceeded'
    ) THEN
        ALTER TYPE submission_status RENAME VALUE 'system_error' TO 'output_exceeded';
    END IF;
END
$do$;

ALTER TYPE submission_status ADD VALUE IF NOT EXISTS 'output_exceeded';

CREATE TABLE IF NOT EXISTS submissions(
    submission_id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    problem_id BIGINT NOT NULL,
    language TEXT NOT NULL,
    source_code TEXT NOT NULL,
    status submission_status NOT NULL DEFAULT 'queued',
    score SMALLINT,
    compile_output TEXT,
    judge_output TEXT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT submissions_score_range_check
        CHECK(score IS NULL OR (score >= 0 AND score <= 100))
);

CREATE TABLE IF NOT EXISTS submission_status_history(
    history_id BIGSERIAL PRIMARY KEY,
    submission_id BIGINT NOT NULL REFERENCES submissions(submission_id) ON DELETE CASCADE,
    from_status submission_status,
    to_status submission_status NOT NULL,
    reason TEXT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS submissions_user_created_idx
    ON submissions(user_id, created_at DESC);

CREATE INDEX IF NOT EXISTS submissions_problem_created_idx
    ON submissions(problem_id, created_at DESC);

CREATE INDEX IF NOT EXISTS submission_status_history_submission_created_idx
    ON submission_status_history(submission_id, created_at DESC);

COMMIT;
SQL

echo "migration completed"
