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
    version INTEGER NOT NULL DEFAULT 1,
    CONSTRAINT problems_version_check CHECK(version > 0)
);

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
VALUES('problem_schema_v1')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v2')
ON CONFLICT(version) DO NOTHING;

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM information_schema.columns
        WHERE
            table_schema = 'public' AND
            table_name = 'problems' AND
            column_name = 'memory_limit_mb'
    ) THEN
        INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms)
        SELECT problem_id, memory_limit_mb, time_limit_ms
        FROM problems
        ON CONFLICT(problem_id) DO NOTHING;
    END IF;
END
$do$;

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM information_schema.columns
        WHERE
            table_schema = 'public' AND
            table_name = 'problems' AND
            column_name = 'submission_count'
    ) THEN
        INSERT INTO problem_statistics(problem_id, submission_count, accepted_count)
        SELECT problem_id, submission_count, accepted_count
        FROM problems
        ON CONFLICT(problem_id) DO NOTHING;
    END IF;
END
$do$;

ALTER TABLE problems
    ADD COLUMN IF NOT EXISTS version INTEGER NOT NULL DEFAULT 1;

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM pg_constraint
        WHERE
            conrelid = 'problems'::regclass AND
            conname = 'problems_version_check'
    ) THEN
        ALTER TABLE problems
            ADD CONSTRAINT problems_version_check CHECK(version > 0);
    END IF;
END
$do$;

DROP INDEX IF EXISTS problems_submission_count_idx;
DROP INDEX IF EXISTS problems_accepted_count_idx;

ALTER TABLE problems
    DROP COLUMN IF EXISTS memory_limit_mb,
    DROP COLUMN IF EXISTS time_limit_ms,
    DROP COLUMN IF EXISTS submission_count,
    DROP COLUMN IF EXISTS accepted_count;

ALTER TABLE problem_statements
    ADD COLUMN IF NOT EXISTS note TEXT;

ALTER TABLE problem_statements
    ADD COLUMN IF NOT EXISTS sample_count INTEGER NOT NULL DEFAULT 0;

ALTER TABLE problem_statements
    ADD COLUMN IF NOT EXISTS testcase_count INTEGER NOT NULL DEFAULT 0;

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM pg_constraint
        WHERE
            conrelid = 'problem_statements'::regclass AND
            conname = 'problem_statements_sample_count_check'
    ) THEN
        ALTER TABLE problem_statements
            ADD CONSTRAINT problem_statements_sample_count_check CHECK(sample_count >= 0);
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
            ADD CONSTRAINT problem_statements_testcase_count_check CHECK(testcase_count >= 0);
    END IF;
END
$do$;

WITH sample_order_aggregate AS(
    SELECT
        problem_id,
        MAX(sample_order)::INTEGER AS max_sample_order
    FROM problem_samples
    GROUP BY problem_id
)
UPDATE problem_statements statement_table
SET
    sample_count = GREATEST(statement_table.sample_count, sample_order_aggregate.max_sample_order),
    updated_at = NOW()
FROM sample_order_aggregate
WHERE statement_table.problem_id = sample_order_aggregate.problem_id;

WITH testcase_count_aggregate AS(
    SELECT
        problem_id,
        COUNT(*)::INTEGER AS testcase_count
    FROM problem_testcases
    GROUP BY problem_id
)
UPDATE problem_statements statement_table
SET
    testcase_count = GREATEST(statement_table.testcase_count, testcase_count_aggregate.testcase_count),
    updated_at = NOW()
FROM testcase_count_aggregate
WHERE statement_table.problem_id = testcase_count_aggregate.problem_id;

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM information_schema.columns
        WHERE
            table_schema = 'public' AND
            table_name = 'problem_statistics' AND
            column_name = 'note'
    ) THEN
        UPDATE problem_statements statement_table
        SET
            note = statistics_table.note,
            updated_at = NOW()
        FROM problem_statistics statistics_table
        WHERE
            statistics_table.problem_id = statement_table.problem_id AND
            statistics_table.note IS NOT NULL AND
            statement_table.note IS DISTINCT FROM statistics_table.note;

        ALTER TABLE problem_statistics
            DROP COLUMN note;
    END IF;
END
$do$;

ALTER TABLE problem_statements
    ALTER COLUMN description SET NOT NULL,
    ALTER COLUMN input_format SET NOT NULL,
    ALTER COLUMN output_format SET NOT NULL;

ALTER TABLE problem_samples
    DROP COLUMN IF EXISTS explanation;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v3')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v4')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v5')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v6')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v7')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v8')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v9')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v10')
ON CONFLICT(version) DO NOTHING;

COMMIT;
SQL

echo "migration completed"
