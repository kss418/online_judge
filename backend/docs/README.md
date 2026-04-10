# docs

- API documents live in [`api/`](./api).
- Schema documents live in [`schema/`](./schema).
- `HTTP_WORKER_COUNT` is optional. If omitted, `http_server` starts one
  worker thread per detected hardware thread.
- `HTTP_HANDLER_WORKER_COUNT` is optional. If omitted, `http_server` uses the
  same size as the resolved HTTP worker count for response handling work.
- `HTTP_DB_POOL_SIZE` is optional. If omitted, `http_server` uses the same
  size as the resolved HTTP worker count.
- `judge_server` sandbox execution requires `nsjail` and the
  `JUDGE_NSJAIL_PATH` / `JUDGE_JAVA_COMPILER_PATH` environment variables.
- `JUDGE_WORKER_COUNT` is optional. If omitted, `judge_server` starts one
  worker thread per detected hardware thread.
- PostgreSQL profiling helpers for `pg_stat_statements`, `log_lock_waits`, and
  `deadlock_timeout` are documented in
  [`db_performance_debugging.md`](./db_performance_debugging.md).
- Public HTTP error handling rules are documented in
  [`error_policy.md`](./error_policy.md).
- A basic backend regression flow suite can be run with
  [`../test/run_basic_flow_tests.sh`](../test/run_basic_flow_tests.sh).

## http handler intent

- HTTP handlers use `*_query_handler` and `*_command_handler` suffixes to
  reflect read/write intent, not HTTP verb alone.
- Query handlers should finish on read-only query/service paths.
- Command handlers should finish on action/write-service paths; any
  read-after-write orchestration belongs in the action/service layer, not the
  handler.
- `POST` endpoints can still be queries when the intent is read-only. The
  current example is submission status batch lookup.

## structure

- [`api/system_api.md`](./api/system_api.md)
- [`api/auth_api.md`](./api/auth_api.md)
- [`api/submission_api.md`](./api/submission_api.md)
- [`api/problem_api.md`](./api/problem_api.md)
- [`error_policy.md`](./error_policy.md)
- [`db_performance_debugging.md`](./db_performance_debugging.md)
- [`schema/user_schema.md`](./schema/user_schema.md)
- [`schema/auth_schema.md`](./schema/auth_schema.md)
- [`schema/problem_schema.md`](./schema/problem_schema.md)
- [`schema/submission_schema.md`](./schema/submission_schema.md)
- [`schema/user_problem_schema.md`](./schema/user_problem_schema.md)
