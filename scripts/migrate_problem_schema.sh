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
    title TEXT NOT NULL DEFAULT 'untitled problem',
    version INTEGER NOT NULL DEFAULT 1,
    CONSTRAINT problems_version_check CHECK(version > 0),
    CONSTRAINT problems_title_check CHECK(char_length(title) > 0)
);

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM information_schema.columns
        WHERE
            table_schema = 'public' AND
            table_name = 'problems' AND
            column_name = 'title'
    ) THEN
        ALTER TABLE problems
            ADD COLUMN title TEXT;
    END IF;
END
$do$;

UPDATE problems
SET title = 'problem ' || problem_id::text
WHERE title IS NULL OR title = '';

ALTER TABLE problems
    ALTER COLUMN title SET NOT NULL;

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM pg_constraint
        WHERE
            conrelid = 'problems'::regclass AND
            conname = 'problems_title_check'
    ) THEN
        ALTER TABLE problems
            ADD CONSTRAINT problems_title_check
            CHECK(char_length(title) > 0);
    END IF;
END
$do$;

CREATE TABLE IF NOT EXISTS problem_limits(
    problem_id BIGINT PRIMARY KEY REFERENCES problems(problem_id) ON DELETE CASCADE,
    memory_limit_mb INTEGER NOT NULL,
    time_limit_ms INTEGER NOT NULL,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT problem_limits_memory_limit_check CHECK(memory_limit_mb > 0),
    CONSTRAINT problem_limits_time_limit_check CHECK(time_limit_ms > 0)
);

CREATE TABLE IF NOT EXISTS problem_statistics(
    problem_id BIGINT PRIMARY KEY REFERENCES problems(problem_id) ON DELETE CASCADE,
    submission_count BIGINT NOT NULL DEFAULT 0,
    accepted_count BIGINT NOT NULL DEFAULT 0,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT problem_statistics_submission_count_check CHECK(submission_count >= 0),
    CONSTRAINT problem_statistics_accepted_count_check CHECK(accepted_count >= 0),
    CONSTRAINT problem_statistics_accepted_not_over_submission_check CHECK(accepted_count <= submission_count)
);

CREATE INDEX IF NOT EXISTS problem_statistics_submission_count_idx
    ON problem_statistics(submission_count DESC);

CREATE INDEX IF NOT EXISTS problem_statistics_accepted_count_idx
    ON problem_statistics(accepted_count DESC);

CREATE TABLE IF NOT EXISTS problem_statements(
    problem_id BIGINT PRIMARY KEY REFERENCES problems(problem_id) ON DELETE CASCADE,
    description TEXT NOT NULL,
    input_format TEXT NOT NULL,
    output_format TEXT NOT NULL,
    sample_count INTEGER NOT NULL DEFAULT 0,
    testcase_count INTEGER NOT NULL DEFAULT 0,
    note TEXT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT problem_statements_sample_count_check CHECK(sample_count >= 0),
    CONSTRAINT problem_statements_testcase_count_check CHECK(testcase_count >= 0)
);

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM information_schema.columns
        WHERE
            table_schema = 'public' AND
            table_name = 'problem_statements' AND
            column_name = 'testcase_count'
    ) THEN
        ALTER TABLE problem_statements
            ADD COLUMN testcase_count INTEGER NOT NULL DEFAULT 0;
    END IF;
END
$do$;

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM pg_constraint
        WHERE
            conrelid = 'problem_statements'::regclass AND
            conname = 'problem_statements_testcase_count_check'
    ) THEN
        ALTER TABLE problem_statements
            ADD CONSTRAINT problem_statements_testcase_count_check
            CHECK(testcase_count >= 0);
    END IF;
END
$do$;

CREATE TABLE IF NOT EXISTS problem_samples(
    sample_id BIGSERIAL PRIMARY KEY,
    problem_id BIGINT NOT NULL REFERENCES problems(problem_id) ON DELETE CASCADE,
    sample_order INTEGER NOT NULL,
    sample_input TEXT NOT NULL,
    sample_output TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT problem_samples_sample_order_check CHECK(sample_order > 0),
    CONSTRAINT problem_samples_problem_id_sample_order_unique UNIQUE(problem_id, sample_order)
);

CREATE INDEX IF NOT EXISTS problem_samples_problem_id_sample_order_idx
    ON problem_samples(problem_id, sample_order ASC);

CREATE TABLE IF NOT EXISTS problem_testcases(
    testcase_id BIGSERIAL PRIMARY KEY,
    problem_id BIGINT NOT NULL REFERENCES problems(problem_id) ON DELETE CASCADE,
    testcase_order INTEGER NOT NULL,
    testcase_input TEXT NOT NULL,
    testcase_output TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT problem_testcases_testcase_order_check CHECK(testcase_order > 0),
    CONSTRAINT problem_testcases_problem_id_testcase_order_unique UNIQUE(problem_id, testcase_order)
);

CREATE INDEX IF NOT EXISTS problem_testcases_problem_id_testcase_order_idx
    ON problem_testcases(problem_id, testcase_order ASC);

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
VALUES('problem_schema_v11')
ON CONFLICT(version) DO NOTHING;

COMMIT;
SQL

echo "migration completed"
