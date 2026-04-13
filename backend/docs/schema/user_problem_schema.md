# user_problem_schema

Base migration: `scripts/migrate_user_problem_schema.sh`
Migration SQL: `scripts/sql/migrate_user_problem_schema.sql`

Prerequisites:

- `auth_schema`
- `problem_schema`
- `submission_schema`

## table

### `user_problem_attempt_summary`

One row per attempted `user_id x problem_id` pair. This is the sparse summary table used for
problem-list style lookups and is intended to be updated incrementally as submissions change.

| column | type | nullable | note |
|---|---|---|---|
| `user_id` | `bigint` | no | fk source: `users(user_id)` |
| `problem_id` | `bigint` | no | fk source: `problems(problem_id)` |
| `submission_count` | `bigint` | no | total submissions by the user for the problem |
| `accepted_submission_count` | `bigint` | no | accepted submissions for the user/problem pair |
| `failed_submission_count` | `bigint` | no | judged non-accepted submissions for the user/problem pair |
| `updated_at` | `timestamptz` | no | last time the summary row was refreshed |

`failed_submission_count` includes these submission statuses:

- `wrong_answer`
- `time_limit_exceeded`
- `memory_limit_exceeded`
- `runtime_error`
- `compile_error`
- `output_exceeded`

`infra_failure` is intentionally excluded because it represents judge infrastructure failure, not a judged user failure.

## view

### `user_wrong_problem_list`

Filtered sparse view over the `user_problem_attempt_summary` table.

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

- `user_problem_attempt_summary` stores incremental aggregates derived from `submissions(user_id, problem_id, status, created_at)`
- `user_wrong_problem_list` is a filtered projection of `user_problem_attempt_summary`

## validation

- `scripts/validate_user_problem_summary.sh` compares `user_problem_attempt_summary` against a fresh `GROUP BY` over `submissions`
- Validation SQL lives in `scripts/sql/validate_user_problem_summary_count.sql` and `scripts/sql/validate_user_problem_summary_details.sql`
- The script accepts `DATABASE_URL`; if unset, it falls back to `DB_USER`, `DB_PASSWORD`, `DB_HOST`, `DB_PORT`, and `DB_NAME`
- Typical usage:
  - `bash ./scripts/validate_user_problem_summary.sh`
  - `DATABASE_URL=postgresql://... bash ./scripts/validate_user_problem_summary.sh`

## rollout

1. Apply `migrate_user_problem_schema.sh` so the summary table exists and is backfilled from `submissions`.
2. Deploy the server code that updates the summary table on submission create, finalize, and rejudge.
3. Run `bash ./scripts/validate_user_problem_summary.sh` to confirm the stored summary matches the live submission history.

## shared table

### `schema_migrations`

Shared table used together with `auth_schema`, `problem_schema`, and `submission_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
