# submission_schema

Base migration: `scripts/migrate_submission_schema.sh`

Prerequisites:

- `auth_schema`
- `problem_schema`

## enum

### `submission_status`

- `queued`
- `judging`
- `accepted`
- `wrong_answer`
- `time_limit_exceeded`
- `memory_limit_exceeded`
- `runtime_error`
- `compile_error`
- `output_exceeded`

## table

### `submissions`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `submission_id` | `bigserial` | no | | pk |
| `user_id` | `bigint` | no | | `users(user_id)` fk (when auth schema is applied) |
| `problem_id` | `bigint` | no | | `problems(problem_id)` fk (when problem schema is applied) |
| `language` | `text` | no | |  |
| `source_code` | `text` | no | |  |
| `status` | `submission_status` | no | `queued` |  |
| `score` | `smallint` | yes | | `0..100` check |
| `compile_output` | `text` | yes | |  |
| `judge_output` | `text` | yes | |  |
| `elapsed_ms` | `bigint` | yes | | max elapsed wall-clock time among executed testcases, `>= 0` |
| `max_rss_kb` | `bigint` | yes | | max resident set size among executed testcases in kilobytes, `>= 0` |
| `created_at` | `timestamptz` | no | `now()` |  |
| `updated_at` | `timestamptz` | no | `now()` |  |

Constraints:

- `submissions_pkey`
- `submissions_score_range_check`
- `submissions_elapsed_ms_check`
- `submissions_max_rss_kb_check`
- `submissions_user_id_fkey`
- `submissions_problem_id_fkey`

Indexes:

- `submissions_user_created_idx (user_id, created_at desc)`
- `submissions_problem_created_idx (problem_id, created_at desc)`

### `submission_status_history`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `history_id` | `bigserial` | no | | pk |
| `submission_id` | `bigint` | no | | fk -> `submissions(submission_id)` on delete cascade |
| `from_status` | `submission_status` | yes | |  |
| `to_status` | `submission_status` | no | |  |
| `reason` | `text` | yes | |  |
| `created_at` | `timestamptz` | no | `now()` |  |

Indexes:

- `submission_status_history_submission_created_idx (submission_id, created_at desc)`

### `submission_queue`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `queue_id` | `bigserial` | no | | pk |
| `submission_id` | `bigint` | no | | unique fk -> `submissions(submission_id)` on delete cascade |
| `priority` | `smallint` | no | `0` | higher value is leased first |
| `attempt_count` | `integer` | no | `0` | `>= 0` check |
| `available_at` | `timestamptz` | no | `now()` |  |
| `leased_until` | `timestamptz` | yes | |  |
| `created_at` | `timestamptz` | no | `now()` |  |

Indexes:

- `submission_queue_available_priority_created_idx (available_at, priority desc, created_at asc)`
- `submission_queue_leased_until_idx (leased_until)`

## cross-schema relation

- `submissions.user_id -> users.user_id`
- `submissions.problem_id -> problems.problem_id`

## shared table

### `schema_migrations`

Shared table used together with `problem_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
