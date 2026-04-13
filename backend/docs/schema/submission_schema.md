# submission_schema

Base migration: `scripts/migrate_submission_schema.sh`
Migration SQL: `scripts/sql/migrate_submission_schema.sql`

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
- `build_resource_exceeded`
- `output_exceeded`
- `infra_failure`

## table

### `submissions`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `submission_id` | `bigserial` | no | | pk |
| `user_id` | `bigint` | no | | `users(user_id)` fk (when auth schema is applied) |
| `problem_id` | `bigint` | no | | `problems(problem_id)` fk (when problem schema is applied) |
| `problem_version` | `integer` | no | | pinned problem version for this submission |
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
| `attempt_no` | `integer` | no | `0` | current lease generation, `>= 0` check |
| `lease_token` | `text` | yes | | current lease owner token |
| `available_at` | `timestamptz` | no | `now()` |  |
| `leased_until` | `timestamptz` | yes | |  |
| `created_at` | `timestamptz` | no | `now()` |  |

Indexes:

- `submission_queue_available_priority_created_idx (available_at, priority desc, created_at asc)`
- `submission_queue_leased_until_idx (leased_until)`

### `judge_instances`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `instance_id` | `text` | no | | pk, per-process judge runtime row |
| `started_at` | `timestamptz` | no | `now()` | judge process start timestamp |
| `last_heartbeat_at` | `timestamptz` | no | `now()` | last runtime heartbeat written by the judge process |
| `configured_worker_count` | `bigint` | no | `0` | configured worker count, `>= 0` |
| `busy_worker_count` | `bigint` | no | `0` | currently busy worker count, `>= 0` |
| `snapshot_cache_hit_count` | `bigint` | no | `0` | cumulative snapshot cache hit count, `>= 0` |
| `snapshot_cache_miss_count` | `bigint` | no | `0` | cumulative snapshot cache miss count, `>= 0` |
| `last_sandbox_self_check_status` | `text` | yes | | latest reported startup self-check status |
| `last_sandbox_self_check_at` | `timestamptz` | yes | | timestamp of latest reported self-check |
| `last_sandbox_self_check_message` | `text` | yes | | failure detail or informational note |

Indexes:

- `judge_instances_last_heartbeat_idx (last_heartbeat_at desc)`
- `judge_instances_last_self_check_idx (last_sandbox_self_check_at desc)`

## cross-schema relation

- `submissions.user_id -> users.user_id`
- `submissions.problem_id -> problems.problem_id`

## notes

- Judge workers lease ownership from `submission_queue`, but the pinned evaluation target is always read from `submissions.problem_version`.
- Basic rejudge re-enqueues the same submission and preserves `submissions.problem_version`, so it re-evaluates the original published problem version.
- HTTP create/rejudge success responses should expose this same pinned `problem_version` value.
- Rejudging against the latest problem version should be implemented as a separate admin workflow rather than mutating the existing submission.
- `/api/system/status` combines `submission_queue` depth with aggregate
  `judge_instances` heartbeats for operational visibility.

## internal build outcome model

The judge build step has an internal outcome classification that is richer than
the persisted `submission_status` enum.

Current internal build outcomes:

- `BuildSuccess`
- `UserCompileError`
- `CompileResourceExceeded`
- `BuildInfraFailure`

Current `CompileResourceExceeded` reason classification:

- `wall_clock`
- `signaled`
- `unknown`

Notes:

- These outcomes are internal judge-layer facts used between builder,
  processor, and lifecycle components.
- They are not stored 1:1 in `submissions.status`.
- `CompileResourceExceeded` currently maps to the external
  `build_resource_exceeded` submission status.
- Memory / OOM is not yet identified precisely enough to expose as a more
  specific stable public status, so it currently remains under
  `build_resource_exceeded` with an internal reason such as
  `CompileResourceExceeded{signaled}` or `CompileResourceExceeded{unknown}`.

## current build policy

Current submission lifecycle policy is:

- `BuildSuccess` -> acquire testcase snapshot -> execute -> evaluate -> finalize
  with the evaluated judge result
- `UserCompileError` -> finalize as `compile_error`
- `CompileResourceExceeded` -> currently finalize as `build_resource_exceeded`
- `BuildInfraFailure` -> keep the existing infra-failure policy
  (`infra_failure` finalize or immediate requeue, depending on lifecycle retry
  policy)

This means internal build classification remains richer than the external
submission status contract because `CompileResourceExceeded` still carries an
internal reason, while public behavior now exposes a dedicated
`build_resource_exceeded` status.

## future evolution

- `CompileResourceExceeded` may later be refined into more specific public
  statuses or reason fields if the current single
  `build_resource_exceeded` status becomes too coarse.
- Retry behavior for some build outcomes, especially compile resource
  exhaustion, may be revisited in a separate change after operational data is
  collected.

## test plan

Minimum regression validation after build outcome policy changes:

1. Run [`test/submission_flow.sh`](../../test/submission_flow.sh).
2. Run [`test/judge_server_flow.sh`](../../test/judge_server_flow.sh).

Recommended scenario coverage:

1. User compile error
   - input: source with a syntax error
   - expect internal outcome `UserCompileError`
   - expect external submission status `compile_error`
2. Compile timeout
   - input: intentionally slow compile source such as heavy C++ template
     expansion
   - expect internal outcome `CompileResourceExceeded{wall_clock}`
   - current external status is `build_resource_exceeded`
3. Build infra failure
   - input: broken toolchain path or unavailable sandbox
   - expect internal outcome `BuildInfraFailure`
   - expect lifecycle infra-failure handling (`infra_failure` finalize or
     immediate requeue)
4. Normal build success
   - input: valid source
   - expect internal outcome `BuildSuccess`
   - expect normal execute/evaluate path

Testing guidance:

- End-to-end tests should stay small and stable.
- Timeout / OOM reproduction can be flaky, so detailed classification should
  prefer pure helper-level tests when possible.
- Flow tests should keep covering the externally visible contract, while
  helper-level tests can cover internal build classification rules.

## shared table

### `schema_migrations`

Shared table used together with `problem_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
