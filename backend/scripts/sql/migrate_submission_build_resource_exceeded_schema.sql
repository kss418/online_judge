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
        WHERE table_schema = 'public' AND table_name = 'submissions'
    ) THEN
        RAISE EXCEPTION 'submission_schema must be applied before submission_build_resource_exceeded_schema';
    END IF;

    IF NOT EXISTS(
        SELECT 1
        FROM information_schema.tables
        WHERE table_schema = 'public' AND table_name = 'user_submission_statistics'
    ) THEN
        RAISE EXCEPTION 'user_statistics_schema must be applied before submission_build_resource_exceeded_schema';
    END IF;
END
$do$;

ALTER TYPE submission_status
    ADD VALUE IF NOT EXISTS 'build_resource_exceeded';

ALTER TABLE user_submission_statistics
    ADD COLUMN IF NOT EXISTS build_resource_exceeded_submission_count BIGINT NOT NULL DEFAULT 0;

ALTER TABLE user_submission_statistics
    DROP CONSTRAINT IF EXISTS user_submission_statistics_non_negative_check;

ALTER TABLE user_submission_statistics
    ADD CONSTRAINT user_submission_statistics_non_negative_check CHECK(
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
    );

ALTER TABLE user_submission_statistics
    DROP CONSTRAINT IF EXISTS user_submission_statistics_total_check;

ALTER TABLE user_submission_statistics
    ADD CONSTRAINT user_submission_statistics_total_check CHECK(
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
    );

UPDATE user_submission_statistics
SET build_resource_exceeded_submission_count =
        aggregated.build_resource_exceeded_submission_count,
    updated_at = NOW()
FROM (
    SELECT
        submissions.user_id,
        COUNT(*) FILTER(
            WHERE submissions.status = 'build_resource_exceeded'::submission_status
        )::BIGINT AS build_resource_exceeded_submission_count
    FROM submissions
    GROUP BY submissions.user_id
) AS aggregated
WHERE user_submission_statistics.user_id = aggregated.user_id;

INSERT INTO schema_migrations(version)
VALUES('submission_build_resource_exceeded_schema_v1')
ON CONFLICT(version) DO NOTHING;

COMMIT;
