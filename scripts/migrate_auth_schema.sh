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

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'auth_users'
    ) AND NOT EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'users'
    ) THEN
        ALTER TABLE auth_users RENAME TO users;
    END IF;
END
$do$;

CREATE TABLE IF NOT EXISTS users(
    user_id BIGINT PRIMARY KEY,
    user_login_id TEXT,
    user_password_hash TEXT,
    is_admin BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS user_login_id TEXT;

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS user_password_hash TEXT;

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM pg_constraint
        WHERE
            conrelid = 'users'::regclass AND
            conname = 'auth_users_pkey'
    ) THEN
        ALTER TABLE users
            RENAME CONSTRAINT auth_users_pkey TO users_pkey;
    END IF;
END
$do$;

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM pg_constraint
        WHERE
            conrelid = 'users'::regclass AND
            conname = 'users_user_login_id_not_blank'
    ) THEN
        ALTER TABLE users
            ADD CONSTRAINT users_user_login_id_not_blank
            CHECK(user_login_id IS NULL OR user_login_id <> '');
    END IF;
END
$do$;

DO $do$
BEGIN
    IF NOT EXISTS(
        SELECT 1
        FROM pg_constraint
        WHERE
            conrelid = 'users'::regclass AND
            conname = 'users_user_password_hash_not_blank'
    ) THEN
        ALTER TABLE users
            ADD CONSTRAINT users_user_password_hash_not_blank
            CHECK(user_password_hash IS NULL OR user_password_hash <> '');
    END IF;
END
$do$;

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
BEGIN
    IF EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'submissions'
    ) THEN
        INSERT INTO users(user_id)
        SELECT DISTINCT user_id
        FROM submissions
        ON CONFLICT(user_id) DO NOTHING;
    END IF;
END
$do$;

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

CREATE INDEX IF NOT EXISTS auth_tokens_expires_at_idx
    ON auth_tokens(expires_at);

CREATE INDEX IF NOT EXISTS auth_tokens_active_user_expires_idx
    ON auth_tokens(user_id, expires_at DESC)
    WHERE revoked_at IS NULL;

INSERT INTO schema_migrations(version)
VALUES('auth_schema_v1')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('auth_schema_v2')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('auth_schema_v3')
ON CONFLICT(version) DO NOTHING;

COMMIT;
SQL

echo "migration completed"
