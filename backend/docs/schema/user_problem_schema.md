# user_problem_schema

Base migration: `scripts/migrate_user_problem_schema.sh`

Prerequisites:

- `auth_schema`
- `problem_schema`
- `submission_schema`

## view

### `user_problem_attempt_summary`

One row per attempted `user_id x problem_id` pair. This is the sparse base view intended for
problem-list style lookups.

| column | type | nullable | note |
|---|---|---|---|
| `user_id` | `bigint` | no | fk source: `users(user_id)` |
| `problem_id` | `bigint` | no | fk source: `problems(problem_id)` |
| `submission_count` | `bigint` | no | total submissions by the user for the problem |
| `accepted_submission_count` | `bigint` | no | accepted submissions for the user/problem pair |
| `failed_submission_count` | `bigint` | no | judged non-accepted submissions for the user/problem pair |

`failed_submission_count` includes these submission statuses:

- `wrong_answer`
- `time_limit_exceeded`
- `memory_limit_exceeded`
- `runtime_error`
- `compile_error`
- `output_exceeded`

### `user_wrong_problem_list`

Filtered sparse view over `user_problem_attempt_summary`.

| column | type | nullable | note |
|---|---|---|---|
| `user_id` | `bigint` | no | fk source: `users(user_id)` |
| `problem_id` | `bigint` | no | fk source: `problems(problem_id)` |
| `submission_count` | `bigint` | no | total submissions by the user for the problem |
| `accepted_submission_count` | `bigint` | no | always `0` in this view |
| `failed_submission_count` | `bigint` | no | failed judged submissions for the user/problem pair |
| `problem_state` | `text` | no | always `wrong` |

This view represents unsolved problems for which the user already has at least one failed judged
submission. Pending-only problems are excluded.

## cross-schema relation

- `user_problem_attempt_summary` aggregates from `submissions(user_id, problem_id, status, created_at)`
- `user_wrong_problem_list` is a filtered projection of `user_problem_attempt_summary`

## shared table

### `schema_migrations`

Shared table used together with `auth_schema`, `problem_schema`, and `submission_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
