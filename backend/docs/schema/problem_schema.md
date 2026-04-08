# problem_schema

Base migration: `scripts/migrate_problem_schema.sh`

## table

### `problems`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `problem_id` | `bigserial` | no | | pk |
| `version` | `integer` | no | `1` | `version > 0` check |

Constraints:

- `problems_pkey`
- `problems_version_check`

### `problem_limits`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `problem_id` | `bigint` | no | | pk, fk -> `problems(problem_id)` on delete cascade |
| `memory_limit_mb` | `integer` | no | | `> 0` check |
| `time_limit_ms` | `integer` | no | | `> 0` check |
| `updated_at` | `timestamptz` | no | `now()` |  |

Constraints:

- `problem_limits_pkey`
- `problem_limits_problem_id_fkey`
- `problem_limits_memory_limit_check`
- `problem_limits_time_limit_check`

### `problem_statistics`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `problem_id` | `bigint` | no | | pk, fk -> `problems(problem_id)` on delete cascade |
| `submission_count` | `bigint` | no | `0` | `>= 0` check |
| `accepted_count` | `bigint` | no | `0` | `>= 0`, `accepted_count <= submission_count` |
| `updated_at` | `timestamptz` | no | `now()` |  |

Indexes:

- `problem_statistics_submission_count_idx (submission_count desc)`
- `problem_statistics_accepted_count_idx (accepted_count desc)`

### `problem_statements`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `problem_id` | `bigint` | no | | pk, fk -> `problems(problem_id)` on delete cascade |
| `description` | `text` | no | |  |
| `input_format` | `text` | no | |  |
| `output_format` | `text` | no | |  |
| `sample_count` | `integer` | no | `0` | `>= 0` check |
| `testcase_count` | `integer` | no | `0` | `>= 0` check |
| `note` | `text` | yes | | optional note for problem statement |
| `created_at` | `timestamptz` | no | `now()` |  |
| `updated_at` | `timestamptz` | no | `now()` |  |

### `problem_samples`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `sample_id` | `bigserial` | no | | pk |
| `problem_id` | `bigint` | no | | fk -> `problems(problem_id)` on delete cascade |
| `sample_order` | `integer` | no | | `> 0` check |
| `sample_input` | `text` | no | |  |
| `sample_output` | `text` | no | |  |
| `created_at` | `timestamptz` | no | `now()` |  |

Constraints:

- `problem_samples_pkey`
- `problem_samples_problem_id_fkey`
- `problem_samples_sample_order_check`
- `problem_samples_problem_id_sample_order_unique (problem_id, sample_order)`

Indexes:

- `problem_samples_problem_id_sample_order_idx (problem_id, sample_order asc)`

### `problem_testcases`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `testcase_id` | `bigserial` | no | | pk |
| `problem_id` | `bigint` | no | | fk -> `problems(problem_id)` on delete cascade |
| `testcase_order` | `integer` | no | | `> 0` check |
| `testcase_input` | `text` | no | |  |
| `testcase_output` | `text` | no | |  |
| `created_at` | `timestamptz` | no | `now()` |  |

Constraints:

- `problem_testcases_pkey`
- `problem_testcases_problem_id_fkey`
- `problem_testcases_testcase_order_check`
- `problem_testcases_problem_id_testcase_order_unique (problem_id, testcase_order)` deferrable, initially immediate

Indexes:

- `problem_testcases_problem_id_testcase_order_idx (problem_id, testcase_order asc)`

### `problem_version_manifests`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `problem_id` | `bigint` | no | | pk part, fk -> `problems(problem_id)` on delete cascade |
| `version` | `integer` | no | | pk part, immutable published problem version |
| `memory_limit_mb` | `integer` | no | | published memory limit snapshot |
| `time_limit_ms` | `integer` | no | | published time limit snapshot |
| `testcase_count` | `integer` | no | `0` | published testcase count snapshot |
| `published_at` | `timestamptz` | no | `now()` | snapshot materialization timestamp |

Constraints:

- `problem_version_manifests_pkey (problem_id, version)`
- `problem_version_manifests_problem_id_fkey`
- `problem_version_manifests_version_check`
- `problem_version_manifests_memory_limit_check`
- `problem_version_manifests_time_limit_check`
- `problem_version_manifests_testcase_count_check`

### `problem_version_testcases`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `problem_id` | `bigint` | no | | pk part |
| `version` | `integer` | no | | pk part |
| `testcase_order` | `integer` | no | | pk part, published testcase order |
| `testcase_input` | `text` | no | | published testcase input |
| `testcase_output` | `text` | no | | published testcase output |
| `input_char_count` | `integer` | no | `0` | published testcase input char count |
| `input_line_count` | `integer` | no | `0` | published testcase input line count |
| `output_char_count` | `integer` | no | `0` | published testcase output char count |
| `output_line_count` | `integer` | no | `0` | published testcase output line count |
| `published_at` | `timestamptz` | no | `now()` | snapshot materialization timestamp |

Constraints:

- `problem_version_testcases_pkey (problem_id, version, testcase_order)`
- `problem_version_testcases_manifest_fkey -> problem_version_manifests(problem_id, version)` on delete cascade
- `problem_version_testcases_version_check`
- `problem_version_testcases_testcase_order_check`
- `problem_version_testcases_input_char_count_check`
- `problem_version_testcases_input_line_count_check`
- `problem_version_testcases_output_char_count_check`
- `problem_version_testcases_output_line_count_check`

Notes:

- Judge snapshot acquisition reads only from `problem_version_manifests` and `problem_version_testcases` by `(problem_id, version)`.
- Mutable current-state tables such as `problem_limits` and `problem_testcases` are published into these immutable tables whenever a problem version is bumped.

## cross-schema relation

- `submissions.problem_id -> problems.problem_id`

## shared table

### `schema_migrations`

Shared table used together with `submission_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
