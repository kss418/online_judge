BEGIN;

CREATE TABLE IF NOT EXISTS schema_migrations(
    version TEXT PRIMARY KEY,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS problems(
    problem_id BIGSERIAL PRIMARY KEY,
    title TEXT NOT NULL DEFAULT 'untitled problem',
    version INTEGER NOT NULL DEFAULT 1,
    CONSTRAINT problems_version_check CHECK(version > 0),
    CONSTRAINT problems_title_check CHECK(char_length(title) > 0)
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
    input_char_count INTEGER NOT NULL DEFAULT 0,
    input_line_count INTEGER NOT NULL DEFAULT 0,
    output_char_count INTEGER NOT NULL DEFAULT 0,
    output_line_count INTEGER NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT problem_testcases_testcase_order_check CHECK(testcase_order > 0),
    CONSTRAINT problem_testcases_problem_id_testcase_order_unique
        UNIQUE(problem_id, testcase_order)
        DEFERRABLE INITIALLY IMMEDIATE
);

ALTER TABLE problem_testcases
    ADD COLUMN IF NOT EXISTS input_char_count INTEGER NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS input_line_count INTEGER NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS output_char_count INTEGER NOT NULL DEFAULT 0,
    ADD COLUMN IF NOT EXISTS output_line_count INTEGER NOT NULL DEFAULT 0;

CREATE INDEX IF NOT EXISTS problem_testcases_problem_id_testcase_order_idx
    ON problem_testcases(problem_id, testcase_order ASC);

CREATE TABLE IF NOT EXISTS problem_version_manifests(
    problem_id BIGINT NOT NULL REFERENCES problems(problem_id) ON DELETE CASCADE,
    version INTEGER NOT NULL,
    memory_limit_mb INTEGER NOT NULL,
    time_limit_ms INTEGER NOT NULL,
    testcase_count INTEGER NOT NULL DEFAULT 0,
    published_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY(problem_id, version),
    CONSTRAINT problem_version_manifests_version_check CHECK(version > 0),
    CONSTRAINT problem_version_manifests_memory_limit_check CHECK(memory_limit_mb > 0),
    CONSTRAINT problem_version_manifests_time_limit_check CHECK(time_limit_ms > 0),
    CONSTRAINT problem_version_manifests_testcase_count_check CHECK(testcase_count >= 0)
);

CREATE TABLE IF NOT EXISTS problem_version_testcases(
    problem_id BIGINT NOT NULL,
    version INTEGER NOT NULL,
    testcase_order INTEGER NOT NULL,
    testcase_input TEXT NOT NULL,
    testcase_output TEXT NOT NULL,
    input_char_count INTEGER NOT NULL DEFAULT 0,
    input_line_count INTEGER NOT NULL DEFAULT 0,
    output_char_count INTEGER NOT NULL DEFAULT 0,
    output_line_count INTEGER NOT NULL DEFAULT 0,
    published_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY(problem_id, version, testcase_order),
    CONSTRAINT problem_version_testcases_manifest_fkey
        FOREIGN KEY(problem_id, version)
        REFERENCES problem_version_manifests(problem_id, version)
        ON DELETE CASCADE,
    CONSTRAINT problem_version_testcases_version_check CHECK(version > 0),
    CONSTRAINT problem_version_testcases_testcase_order_check CHECK(testcase_order > 0),
    CONSTRAINT problem_version_testcases_input_char_count_check CHECK(input_char_count >= 0),
    CONSTRAINT problem_version_testcases_input_line_count_check CHECK(input_line_count >= 0),
    CONSTRAINT problem_version_testcases_output_char_count_check CHECK(output_char_count >= 0),
    CONSTRAINT problem_version_testcases_output_line_count_check CHECK(output_line_count >= 0)
);

UPDATE problem_testcases
SET
    input_char_count = char_length(testcase_input),
    input_line_count = CASE
        WHEN testcase_input = '' THEN 0
        ELSE 1 + char_length(testcase_input) - char_length(replace(testcase_input, E'\n', ''))
    END,
    output_char_count = char_length(testcase_output),
    output_line_count = CASE
        WHEN testcase_output = '' THEN 0
        ELSE 1 + char_length(testcase_output) - char_length(replace(testcase_output, E'\n', ''))
    END;

INSERT INTO problem_limits(problem_id, memory_limit_mb, time_limit_ms, updated_at)
SELECT p.problem_id, 256, 1000, NOW()
FROM problems AS p
LEFT JOIN problem_limits AS l
    ON l.problem_id = p.problem_id
WHERE l.problem_id IS NULL
ON CONFLICT(problem_id) DO NOTHING;

INSERT INTO problem_statistics(problem_id, submission_count, accepted_count, updated_at)
SELECT p.problem_id, 0, 0, NOW()
FROM problems AS p
LEFT JOIN problem_statistics AS s
    ON s.problem_id = p.problem_id
WHERE s.problem_id IS NULL
ON CONFLICT(problem_id) DO NOTHING;

INSERT INTO problem_statements(
    problem_id,
    description,
    input_format,
    output_format,
    sample_count,
    testcase_count,
    note,
    created_at,
    updated_at
)
SELECT
    p.problem_id,
    '',
    '',
    '',
    0,
    0,
    NULL,
    NOW(),
    NOW()
FROM problems AS p
LEFT JOIN problem_statements AS st
    ON st.problem_id = p.problem_id
WHERE st.problem_id IS NULL
ON CONFLICT(problem_id) DO NOTHING;

INSERT INTO problem_version_manifests(
    problem_id,
    version,
    memory_limit_mb,
    time_limit_ms,
    testcase_count,
    published_at
)
SELECT
    problem_table.problem_id,
    problem_table.version,
    limits_table.memory_limit_mb,
    limits_table.time_limit_ms,
    COALESCE(testcase_counts.testcase_count, 0),
    NOW()
FROM problems AS problem_table
JOIN problem_limits AS limits_table
    ON limits_table.problem_id = problem_table.problem_id
LEFT JOIN (
    SELECT
        problem_id,
        COUNT(*)::INTEGER AS testcase_count
    FROM problem_testcases
    GROUP BY problem_id
) AS testcase_counts
    ON testcase_counts.problem_id = problem_table.problem_id
ON CONFLICT(problem_id, version) DO UPDATE
SET
    memory_limit_mb = EXCLUDED.memory_limit_mb,
    time_limit_ms = EXCLUDED.time_limit_ms,
    testcase_count = EXCLUDED.testcase_count,
    published_at = NOW();

INSERT INTO problem_version_testcases(
    problem_id,
    version,
    testcase_order,
    testcase_input,
    testcase_output,
    input_char_count,
    input_line_count,
    output_char_count,
    output_line_count,
    published_at
)
SELECT
    problem_table.problem_id,
    problem_table.version,
    testcase_table.testcase_order,
    testcase_table.testcase_input,
    testcase_table.testcase_output,
    testcase_table.input_char_count,
    testcase_table.input_line_count,
    testcase_table.output_char_count,
    testcase_table.output_line_count,
    NOW()
FROM problems AS problem_table
JOIN problem_testcases AS testcase_table
    ON testcase_table.problem_id = problem_table.problem_id
ON CONFLICT(problem_id, version, testcase_order) DO UPDATE
SET
    testcase_input = EXCLUDED.testcase_input,
    testcase_output = EXCLUDED.testcase_output,
    input_char_count = EXCLUDED.input_char_count,
    input_line_count = EXCLUDED.input_line_count,
    output_char_count = EXCLUDED.output_char_count,
    output_line_count = EXCLUDED.output_line_count,
    published_at = NOW();

INSERT INTO schema_migrations(version)
VALUES('problem_schema_v15')
ON CONFLICT(version) DO NOTHING;

COMMIT;
