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

echo "apply user_schema"
psql "${database_url}" \
    -v ON_ERROR_STOP=1 <<'SQL'
BEGIN;

CREATE TABLE IF NOT EXISTS schema_migrations(
    version TEXT PRIMARY KEY,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS user_info(
    user_id BIGSERIAL PRIMARY KEY,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

ALTER TABLE user_info
    ADD COLUMN IF NOT EXISTS updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW();

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'users'
    ) THEN
        IF EXISTS(
            SELECT 1
            FROM information_schema.columns
            WHERE
                table_schema = 'public' AND
                table_name = 'users' AND
                column_name = 'created_at'
        ) THEN
            INSERT INTO user_info(user_id, created_at)
            SELECT user_id, created_at
            FROM users
            ON CONFLICT(user_id) DO NOTHING;
        ELSE
            INSERT INTO user_info(user_id)
            SELECT user_id
            FROM users
            ON CONFLICT(user_id) DO NOTHING;
        END IF;
    END IF;
END
$do$;

DO $do$
DECLARE
    user_info_sequence TEXT;
    max_user_id BIGINT;
BEGIN
    SELECT pg_get_serial_sequence('user_info', 'user_id')
    INTO user_info_sequence;

    IF user_info_sequence IS NULL THEN
        RETURN;
    END IF;

    SELECT COALESCE(MAX(user_id), 0)
    INTO max_user_id
    FROM user_info;

    IF max_user_id >= 1 THEN
        EXECUTE format(
            'SELECT setval(%L, %s, true)',
            user_info_sequence,
            max_user_id
        );
        RETURN;
    END IF;

    EXECUTE format(
        'SELECT setval(%L, 1, false)',
        user_info_sequence
    );
END
$do$;

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM schema_migrations
        WHERE version = 'user_schema_v2'
    ) THEN
        RETURN;
    END IF;

    UPDATE user_info
    SET updated_at = created_at
    WHERE updated_at IS DISTINCT FROM created_at;
END
$do$;

INSERT INTO schema_migrations(version)
VALUES('user_schema_v1')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('user_schema_v2')
ON CONFLICT(version) DO NOTHING;

COMMIT;
SQL

echo "migration completed"
