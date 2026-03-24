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

echo "apply auth_schema"
psql "${database_url}" \
    -v ON_ERROR_STOP=1 <<'SQL'
BEGIN;

CREATE TABLE IF NOT EXISTS schema_migrations(
    version TEXT PRIMARY KEY,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS users(
    user_id BIGSERIAL PRIMARY KEY,
    user_name TEXT NOT NULL,
    user_login_id TEXT,
    user_password_hash TEXT,
    is_admin BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT users_user_name_not_blank CHECK(user_name <> ''),
    CONSTRAINT users_user_login_id_not_blank CHECK(user_login_id IS NULL OR user_login_id <> ''),
    CONSTRAINT users_user_password_hash_not_blank
        CHECK(user_password_hash IS NULL OR user_password_hash <> '')
);

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS user_name TEXT;

UPDATE users
SET user_name = COALESCE(NULLIF(user_login_id, ''), 'user_' || user_id::TEXT)
WHERE user_name IS NULL OR user_name = '';

ALTER TABLE users
    ALTER COLUMN user_name SET NOT NULL;

CREATE TABLE IF NOT EXISTS auth_tokens(
    token_id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    token_hash TEXT NOT NULL,
    issued_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    expires_at TIMESTAMPTZ NOT NULL,
    revoked_at TIMESTAMPTZ,
    last_used_at TIMESTAMPTZ,
    CONSTRAINT auth_tokens_token_hash_unique UNIQUE(token_hash),
    CONSTRAINT auth_tokens_token_hash_not_blank CHECK(token_hash <> ''),
    CONSTRAINT auth_tokens_expires_at_order_check CHECK(expires_at >= issued_at),
    CONSTRAINT auth_tokens_last_used_at_order_check
        CHECK(last_used_at IS NULL OR last_used_at >= issued_at),
    CONSTRAINT auth_tokens_revoked_at_order_check
        CHECK(revoked_at IS NULL OR revoked_at >= issued_at)
);

DO $do$
DECLARE
    duplicate_user_name_row RECORD;
    deduplicated_user_name TEXT;
BEGIN
    FOR duplicate_user_name_row IN
        SELECT user_id, user_name
        FROM (
            SELECT
                user_id,
                user_name,
                ROW_NUMBER() OVER(
                    PARTITION BY user_name
                    ORDER BY user_id
                ) AS duplicate_rank
            FROM users
        ) duplicate_user_name_values
        WHERE duplicate_rank > 1
    LOOP
        deduplicated_user_name :=
            duplicate_user_name_row.user_name || '__dedup_' || duplicate_user_name_row.user_id::TEXT;

        WHILE EXISTS(
            SELECT 1
            FROM users
            WHERE
                user_id <> duplicate_user_name_row.user_id AND
                user_name = deduplicated_user_name
        ) LOOP
            deduplicated_user_name := deduplicated_user_name || '_';
        END LOOP;

        UPDATE users
        SET user_name = deduplicated_user_name
        WHERE user_id = duplicate_user_name_row.user_id;
    END LOOP;

    IF NOT EXISTS(
        SELECT 1
        FROM pg_constraint
        WHERE
            conrelid = 'users'::regclass AND
            conname = 'users_user_name_not_blank'
    ) THEN
        ALTER TABLE users
            ADD CONSTRAINT users_user_name_not_blank
            CHECK(user_name <> '');
    END IF;

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
                conname = 'submissions_user_id_fkey'
        ) THEN
            ALTER TABLE submissions
                ADD CONSTRAINT submissions_user_id_fkey
                FOREIGN KEY(user_id)
                REFERENCES users(user_id);
        END IF;
    END IF;
END
$do$;

CREATE INDEX IF NOT EXISTS auth_tokens_user_issued_idx
    ON auth_tokens(user_id, issued_at DESC);

CREATE UNIQUE INDEX IF NOT EXISTS users_user_login_id_unique_idx
    ON users(user_login_id)
    WHERE user_login_id IS NOT NULL;

CREATE UNIQUE INDEX IF NOT EXISTS users_user_name_unique_idx
    ON users(user_name);

CREATE INDEX IF NOT EXISTS auth_tokens_expires_at_idx
    ON auth_tokens(expires_at);

CREATE INDEX IF NOT EXISTS auth_tokens_active_user_expires_idx
    ON auth_tokens(user_id, expires_at DESC)
    WHERE revoked_at IS NULL;

INSERT INTO schema_migrations(version)
VALUES('auth_schema_v6')
ON CONFLICT(version) DO NOTHING;

COMMIT;
SQL

echo "migration completed"
