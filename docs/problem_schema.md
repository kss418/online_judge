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
| `explanation` | `text` | yes | |  |
| `created_at` | `timestamptz` | no | `now()` |  |

Constraints:

- `problem_samples_pkey`
- `problem_samples_problem_id_fkey`
- `problem_samples_sample_order_check`
- `problem_samples_problem_id_sample_order_unique (problem_id, sample_order)`

Indexes:

- `problem_samples_problem_id_sample_order_idx (problem_id, sample_order asc)`

## cross-schema relation

- `submissions.problem_id -> problems.problem_id`

## shared table

### `schema_migrations`

Shared table used together with `submission_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
