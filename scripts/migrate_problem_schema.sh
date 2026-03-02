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

echo "apply problem_schema"
psql "${database_url}" \
    -v ON_ERROR_STOP=1 <<'SQL'
BEGIN;

CREATE TABLE IF NOT EXISTS schema_migrations(
    version TEXT PRIMARY KEY,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS problems(
    problem_id BIGSERIAL PRIMARY KEY,
    memory_limit_mb INTEGER NOT NULL,
    time_limit_ms INTEGER NOT NULL,
    submission_count BIGINT NOT NULL DEFAULT 0,
    accepted_count BIGINT NOT NULL DEFAULT 0,
    CONSTRAINT problems_memory_limit_check CHECK(memory_limit_mb > 0),
    CONSTRAINT problems_time_limit_check CHECK(time_limit_ms > 0),
    CONSTRAINT problems_submission_count_check CHECK(submission_count >= 0),
    CONSTRAINT problems_accepted_count_check CHECK(accepted_count >= 0),
    CONSTRAINT problems_accepted_not_over_submission_check CHECK(accepted_count <= submission_count)
);

CREATE INDEX IF NOT EXISTS problems_submission_count_idx
    ON problems(submission_count DESC);

CREATE INDEX IF NOT EXISTS problems_accepted_count_idx
    ON problems(accepted_count DESC);

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'submissions'
    ) THEN
        IF NOT EXISTS(
            SELECT 1
            FROM pg_constraint
            WHERE
                conrelid = 'submissions'::regclass AND
                conname = 'submissions_problem_id_fkey'
        ) THEN
            ALTER TABLE submissions
                ADD CONSTRAINT submissions_problem_id_fkey
                FOREIGN KEY(problem_id)
                REFERENCES problems(problem_id);
        END IF;
    END IF;
END
$do$;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v1')
ON CONFLICT(version) DO NOTHING;

COMMIT;
SQL

echo "migration completed"
