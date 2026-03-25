# user_problem_schema

Base migration: `scripts/migrate_user_problem_schema.sh`

Prerequisites:

- `auth_schema`
- `problem_schema`
- `submission_schema`

## view

### `user_problem_list`

One row per `users x problems` pair. This view summarizes each user's current state for every
problem, including unattempted problems.

| column | type | nullable | note |
|---|---|---|---|
| `user_id` | `bigint` | no | fk source: `users(user_id)` |
| `user_name` | `text` | no | copied from `users.user_name` |
| `problem_id` | `bigint` | no | fk source: `problems(problem_id)` |
| `problem_title` | `text` | no | copied from `problems.title` |
| `problem_version` | `integer` | no | copied from `problems.version` |
| `submission_count` | `bigint` | no | total submissions by the user for the problem |
| `accepted_submission_count` | `bigint` | no | accepted submissions for the user/problem pair |
| `failed_submission_count` | `bigint` | no | judged non-accepted submissions for the user/problem pair |
| `problem_state` | `text` | no | one of `unattempted`, `wrong`, `solved` |

`problem_state` is derived with the following priority:

- `solved`: `accepted_submission_count > 0`
- `wrong`: no accepted submission exists and `failed_submission_count > 0`
- `unattempted`: no accepted or failed submission exists for the user/problem pair

`failed_submission_count` includes these submission statuses:

- `wrong_answer`
- `time_limit_exceeded`
- `memory_limit_exceeded`
- `runtime_error`
- `compile_error`
- `output_exceeded`

### `user_wrong_problem_list`

Filtered view over `user_problem_list`.

| column | type | nullable | note |
|---|---|---|---|
| all columns from `user_problem_list` | same as source view | same as source view | only rows with `problem_state = 'wrong'` are exposed |

This view represents unsolved problems for which the user already has at least one failed judged
submission. Pending-only problems are excluded.

## cross-schema relation

- `user_problem_list.user_id -> users.user_id`
- `user_problem_list.problem_id -> problems.problem_id`
- `user_problem_list` aggregates from `submissions(user_id, problem_id, status, created_at)`
- `user_wrong_problem_list` is a filtered projection of `user_problem_list`

## shared table

### `schema_migrations`

Shared table used together with `auth_schema`, `problem_schema`, and `submission_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
