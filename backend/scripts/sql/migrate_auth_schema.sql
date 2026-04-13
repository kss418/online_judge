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

CREATE TABLE IF NOT EXISTS auth_tokens(
    token_id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    token_hash TEXT NOT NULL,
    issued_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    expires_at TIMESTAMPTZ NOT NULL,
    revoked_at TIMESTAMPTZ,
    CONSTRAINT auth_tokens_token_hash_unique UNIQUE(token_hash),
    CONSTRAINT auth_tokens_token_hash_not_blank CHECK(token_hash <> ''),
    CONSTRAINT auth_tokens_expires_at_order_check CHECK(expires_at >= issued_at),
    CONSTRAINT auth_tokens_revoked_at_order_check
        CHECK(revoked_at IS NULL OR revoked_at >= issued_at)
);

CREATE INDEX IF NOT EXISTS auth_tokens_user_issued_idx
    ON auth_tokens(user_id, issued_at DESC);

CREATE UNIQUE INDEX IF NOT EXISTS users_user_login_id_unique_idx
    ON users(user_login_id);

CREATE INDEX IF NOT EXISTS auth_tokens_expires_at_idx
    ON auth_tokens(expires_at);

CREATE INDEX IF NOT EXISTS auth_tokens_active_user_expires_idx
    ON auth_tokens(user_id, expires_at DESC)
    WHERE revoked_at IS NULL;

INSERT INTO schema_migrations(version)
VALUES('auth_schema_v13')
ON CONFLICT(version) DO NOTHING;

COMMIT;
