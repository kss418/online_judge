BEGIN;

CREATE TABLE IF NOT EXISTS schema_migrations(
    version TEXT PRIMARY KEY,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS user_info(
    user_id BIGSERIAL PRIMARY KEY,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    submission_banned_until TIMESTAMPTZ
);

INSERT INTO schema_migrations(version)
VALUES('user_schema_v3')
ON CONFLICT(version) DO NOTHING;

COMMIT;
