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
    IF NOT EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'user_info'
    ) THEN
        RAISE EXCEPTION 'user_schema must be applied before auth_schema';
    END IF;
END
$do$;

CREATE TABLE IF NOT EXISTS users(
    user_id BIGINT PRIMARY KEY,
    user_login_id TEXT NOT NULL,
    user_password_hash TEXT,
    permission_level INTEGER NOT NULL DEFAULT 0,
    auth_updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT users_user_id_fkey
        FOREIGN KEY(user_id)
        REFERENCES user_info(user_id)
        ON DELETE CASCADE,
    CONSTRAINT users_user_login_id_not_blank CHECK(user_login_id <> ''),
    CONSTRAINT users_user_password_hash_not_blank
        CHECK(user_password_hash IS NULL OR user_password_hash <> '')
);

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS user_login_id TEXT;

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS permission_level INTEGER;

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM information_schema.columns
        WHERE
            table_schema = 'public' AND
            table_name = 'users' AND
            column_name = 'updated_at'
    ) AND NOT EXISTS(
        SELECT 1
        FROM information_schema.columns
        WHERE
            table_schema = 'public' AND
            table_name = 'users' AND
            column_name = 'auth_updated_at'
    ) THEN
        ALTER TABLE users
            RENAME COLUMN updated_at TO auth_updated_at;
    END IF;
END
$do$;

ALTER TABLE users
    ADD COLUMN IF NOT EXISTS auth_updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW();

DO $do$
BEGIN
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
END
$do$;

DO $do$
BEGIN
    IF EXISTS(
        SELECT 1
        FROM information_schema.columns
        WHERE
            table_schema = 'public' AND
            table_name = 'users' AND
            column_name = 'user_name'
    ) THEN
        EXECUTE $sql$
            UPDATE users
            SET user_login_id = COALESCE(
                NULLIF(user_login_id, ''),
                NULLIF(user_name, ''),
                'user_' || user_id::TEXT
            )
            WHERE user_login_id IS NULL OR user_login_id = ''
        $sql$;
    ELSE
        EXECUTE $sql$
            UPDATE users
            SET user_login_id = COALESCE(
                NULLIF(user_login_id, ''),
                'user_' || user_id::TEXT
            )
            WHERE user_login_id IS NULL OR user_login_id = ''
        $sql$;
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
            table_name = 'users' AND
            column_name = 'is_admin'
    ) THEN
        EXECUTE $sql$
            UPDATE users
            SET permission_level = CASE WHEN is_admin THEN 1 ELSE 0 END
            WHERE permission_level IS NULL
        $sql$;
    END IF;
END
$do$;

UPDATE users
SET permission_level = CASE
    WHEN permission_level >= 100 THEN 2
    WHEN permission_level >= 10 THEN 1
    WHEN permission_level >= 2 THEN 2
    WHEN permission_level >= 1 THEN 1
    ELSE 0
END
WHERE permission_level IS DISTINCT FROM CASE
    WHEN permission_level >= 100 THEN 2
    WHEN permission_level >= 10 THEN 1
    WHEN permission_level >= 2 THEN 2
    WHEN permission_level >= 1 THEN 1
    ELSE 0
END;

DO $do$
DECLARE
    duplicate_user_login_id_row RECORD;
    deduplicated_user_login_id TEXT;
BEGIN
    FOR duplicate_user_login_id_row IN
        SELECT user_id, user_login_id
        FROM (
            SELECT
                user_id,
                user_login_id,
                ROW_NUMBER() OVER(
                    PARTITION BY user_login_id
                    ORDER BY user_id
                ) AS duplicate_rank
            FROM users
        ) duplicate_user_login_id_values
        WHERE duplicate_rank > 1
    LOOP
        deduplicated_user_login_id :=
            duplicate_user_login_id_row.user_login_id ||
            '__dedup_' ||
            duplicate_user_login_id_row.user_id::TEXT;

        WHILE EXISTS(
            SELECT 1
            FROM users
            WHERE
                user_id <> duplicate_user_login_id_row.user_id AND
                user_login_id = deduplicated_user_login_id
        ) LOOP
            deduplicated_user_login_id := deduplicated_user_login_id || '_';
        END LOOP;

        UPDATE users
        SET user_login_id = deduplicated_user_login_id
        WHERE user_id = duplicate_user_login_id_row.user_id;
    END LOOP;
END
$do$;

ALTER TABLE users
    ALTER COLUMN user_login_id SET NOT NULL;

ALTER TABLE users
    ALTER COLUMN user_id DROP DEFAULT;

ALTER TABLE users
    ALTER COLUMN permission_level SET DEFAULT 0;

ALTER TABLE users
    ALTER COLUMN permission_level SET NOT NULL;

ALTER TABLE users
    DROP CONSTRAINT IF EXISTS users_permission_level_check;

ALTER TABLE users
    ADD CONSTRAINT users_permission_level_check
        CHECK(permission_level BETWEEN 0 AND 2);

ALTER TABLE users
    DROP CONSTRAINT IF EXISTS users_user_name_not_blank;

ALTER TABLE users
    DROP CONSTRAINT IF EXISTS users_user_login_id_not_blank;

ALTER TABLE users
    DROP CONSTRAINT IF EXISTS users_user_id_fkey;

ALTER TABLE users
    ADD CONSTRAINT users_user_login_id_not_blank
        CHECK(user_login_id <> '');

ALTER TABLE users
    ADD CONSTRAINT users_user_id_fkey
        FOREIGN KEY(user_id)
        REFERENCES user_info(user_id)
        ON DELETE CASCADE;

ALTER TABLE users
    DROP COLUMN IF EXISTS is_admin;

DROP INDEX IF EXISTS users_user_name_unique_idx;

ALTER TABLE users
    DROP COLUMN IF EXISTS user_name;

ALTER TABLE users
    DROP COLUMN IF EXISTS created_at;

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

DROP INDEX IF EXISTS users_user_login_id_unique_idx;

CREATE UNIQUE INDEX IF NOT EXISTS users_user_login_id_unique_idx
    ON users(user_login_id);

CREATE INDEX IF NOT EXISTS auth_tokens_expires_at_idx
    ON auth_tokens(expires_at);

CREATE INDEX IF NOT EXISTS auth_tokens_active_user_expires_idx
    ON auth_tokens(user_id, expires_at DESC)
    WHERE revoked_at IS NULL;

INSERT INTO schema_migrations(version)
VALUES('auth_schema_v11')
ON CONFLICT(version) DO NOTHING;

INSERT INTO schema_migrations(version)
VALUES('auth_schema_v12')
ON CONFLICT(version) DO NOTHING;

COMMIT;
SQL

echo "migration completed"
