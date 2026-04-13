WITH expected AS (
    SELECT
        submission_table.user_id,
        submission_table.problem_id,
        COUNT(*)::BIGINT AS submission_count,
        COUNT(*) FILTER(
            WHERE submission_table.status = 'accepted'::submission_status
        )::BIGINT AS accepted_submission_count,
        COUNT(*) FILTER(
            WHERE submission_table.status IN (
                'wrong_answer'::submission_status,
                'time_limit_exceeded'::submission_status,
                'memory_limit_exceeded'::submission_status,
                'runtime_error'::submission_status,
                'compile_error'::submission_status,
                'build_resource_exceeded'::submission_status,
                'output_exceeded'::submission_status
            )
        )::BIGINT AS failed_submission_count
    FROM submissions submission_table
    GROUP BY submission_table.user_id, submission_table.problem_id
)
SELECT
    COALESCE(actual.user_id, expected.user_id) AS user_id,
    COALESCE(actual.problem_id, expected.problem_id) AS problem_id,
    actual.submission_count AS actual_submission_count,
    expected.submission_count AS expected_submission_count,
    actual.accepted_submission_count AS actual_accepted_submission_count,
    expected.accepted_submission_count AS expected_accepted_submission_count,
    actual.failed_submission_count AS actual_failed_submission_count,
    expected.failed_submission_count AS expected_failed_submission_count
FROM user_problem_attempt_summary actual
FULL OUTER JOIN expected
  ON actual.user_id = expected.user_id
 AND actual.problem_id = expected.problem_id
WHERE
    actual.user_id IS NULL OR
    expected.user_id IS NULL OR
    actual.submission_count <> expected.submission_count OR
    actual.accepted_submission_count <> expected.accepted_submission_count OR
    actual.failed_submission_count <> expected.failed_submission_count
ORDER BY user_id, problem_id;
