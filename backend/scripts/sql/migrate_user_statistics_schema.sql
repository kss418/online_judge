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
        WHERE table_schema = 'public' AND table_name = 'users'
    ) THEN
        RAISE EXCEPTION 'auth_schema must be applied before user_statistics_schema';
    END IF;

    IF NOT EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'submissions'
    ) THEN
        RAISE EXCEPTION 'submission_schema must be applied before user_statistics_schema';
    END IF;
END
$do$;

CREATE TABLE IF NOT EXISTS user_submission_statistics(
    user_id BIGINT PRIMARY KEY REFERENCES users(user_id) ON DELETE CASCADE,
    submission_count BIGINT NOT NULL DEFAULT 0,
    queued_submission_count BIGINT NOT NULL DEFAULT 0,
    judging_submission_count BIGINT NOT NULL DEFAULT 0,
    accepted_submission_count BIGINT NOT NULL DEFAULT 0,
    wrong_answer_submission_count BIGINT NOT NULL DEFAULT 0,
    time_limit_exceeded_submission_count BIGINT NOT NULL DEFAULT 0,
    memory_limit_exceeded_submission_count BIGINT NOT NULL DEFAULT 0,
    runtime_error_submission_count BIGINT NOT NULL DEFAULT 0,
    compile_error_submission_count BIGINT NOT NULL DEFAULT 0,
    build_resource_exceeded_submission_count BIGINT NOT NULL DEFAULT 0,
    output_exceeded_submission_count BIGINT NOT NULL DEFAULT 0,
    infra_failure_submission_count BIGINT NOT NULL DEFAULT 0,
    last_submission_at TIMESTAMPTZ,
    last_accepted_at TIMESTAMPTZ,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT user_submission_statistics_non_negative_check CHECK(
        submission_count >= 0 AND
        queued_submission_count >= 0 AND
        judging_submission_count >= 0 AND
        accepted_submission_count >= 0 AND
        wrong_answer_submission_count >= 0 AND
        time_limit_exceeded_submission_count >= 0 AND
        memory_limit_exceeded_submission_count >= 0 AND
        runtime_error_submission_count >= 0 AND
        compile_error_submission_count >= 0 AND
        build_resource_exceeded_submission_count >= 0 AND
        output_exceeded_submission_count >= 0 AND
        infra_failure_submission_count >= 0
    ),
    CONSTRAINT user_submission_statistics_total_check CHECK(
        submission_count =
            queued_submission_count +
            judging_submission_count +
            accepted_submission_count +
            wrong_answer_submission_count +
            time_limit_exceeded_submission_count +
            memory_limit_exceeded_submission_count +
            runtime_error_submission_count +
            compile_error_submission_count +
            build_resource_exceeded_submission_count +
            output_exceeded_submission_count +
            infra_failure_submission_count
    )
);

CREATE OR REPLACE FUNCTION ensure_user_submission_statistics_row()
RETURNS TRIGGER
LANGUAGE plpgsql
AS $function$
BEGIN
    INSERT INTO user_submission_statistics(user_id)
    VALUES(NEW.user_id)
    ON CONFLICT(user_id) DO NOTHING;

    RETURN NEW;
END
$function$;

DROP TRIGGER IF EXISTS users_insert_user_submission_statistics ON users;

CREATE TRIGGER users_insert_user_submission_statistics
AFTER INSERT ON users
FOR EACH ROW
EXECUTE FUNCTION ensure_user_submission_statistics_row();

INSERT INTO user_submission_statistics(
    user_id,
    submission_count,
    queued_submission_count,
    judging_submission_count,
    accepted_submission_count,
    wrong_answer_submission_count,
    time_limit_exceeded_submission_count,
    memory_limit_exceeded_submission_count,
    runtime_error_submission_count,
    compile_error_submission_count,
    build_resource_exceeded_submission_count,
    output_exceeded_submission_count,
    infra_failure_submission_count,
    last_submission_at,
    last_accepted_at,
    updated_at
)
SELECT
    user_table.user_id,
    COUNT(submission_table.submission_id)::BIGINT AS submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'queued'::submission_status
    )::BIGINT AS queued_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'judging'::submission_status
    )::BIGINT AS judging_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'accepted'::submission_status
    )::BIGINT AS accepted_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'wrong_answer'::submission_status
    )::BIGINT AS wrong_answer_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'time_limit_exceeded'::submission_status
    )::BIGINT AS time_limit_exceeded_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'memory_limit_exceeded'::submission_status
    )::BIGINT AS memory_limit_exceeded_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'runtime_error'::submission_status
    )::BIGINT AS runtime_error_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'compile_error'::submission_status
    )::BIGINT AS compile_error_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'build_resource_exceeded'::submission_status
    )::BIGINT AS build_resource_exceeded_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'output_exceeded'::submission_status
    )::BIGINT AS output_exceeded_submission_count,
    COUNT(submission_table.submission_id) FILTER(
        WHERE submission_table.status = 'infra_failure'::submission_status
    )::BIGINT AS infra_failure_submission_count,
    MAX(submission_table.created_at) AS last_submission_at,
    MAX(submission_table.created_at) FILTER(
        WHERE submission_table.status = 'accepted'::submission_status
    ) AS last_accepted_at,
    NOW() AS updated_at
FROM users user_table
LEFT JOIN submissions submission_table
    ON submission_table.user_id = user_table.user_id
GROUP BY user_table.user_id
ON CONFLICT(user_id) DO UPDATE
SET
    submission_count = EXCLUDED.submission_count,
    queued_submission_count = EXCLUDED.queued_submission_count,
    judging_submission_count = EXCLUDED.judging_submission_count,
    accepted_submission_count = EXCLUDED.accepted_submission_count,
    wrong_answer_submission_count = EXCLUDED.wrong_answer_submission_count,
    time_limit_exceeded_submission_count = EXCLUDED.time_limit_exceeded_submission_count,
    memory_limit_exceeded_submission_count = EXCLUDED.memory_limit_exceeded_submission_count,
    runtime_error_submission_count = EXCLUDED.runtime_error_submission_count,
    compile_error_submission_count = EXCLUDED.compile_error_submission_count,
    build_resource_exceeded_submission_count =
        EXCLUDED.build_resource_exceeded_submission_count,
    output_exceeded_submission_count = EXCLUDED.output_exceeded_submission_count,
    infra_failure_submission_count = EXCLUDED.infra_failure_submission_count,
    last_submission_at = EXCLUDED.last_submission_at,
    last_accepted_at = EXCLUDED.last_accepted_at,
    updated_at = EXCLUDED.updated_at;

INSERT INTO schema_migrations(version)
VALUES('user_statistics_schema_v1')
ON CONFLICT(version) DO NOTHING;

COMMIT;
