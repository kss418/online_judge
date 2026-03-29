# docs

- API documents live in [`api/`](./api).
- Schema documents live in [`schema/`](./schema).
- `judge_server` sandbox execution requires `nsjail` and the
  `JUDGE_NSJAIL_PATH` / `JUDGE_JAVA_COMPILER_PATH` environment variables.
- `JUDGE_WORKER_COUNT` is optional. If omitted, `judge_server` starts one
  worker thread per detected hardware thread.

## structure

- [`api/system_api.md`](./api/system_api.md)
- [`api/auth_api.md`](./api/auth_api.md)
- [`api/submission_api.md`](./api/submission_api.md)
- [`api/problem_api.md`](./api/problem_api.md)
- [`schema/user_schema.md`](./schema/user_schema.md)
- [`schema/auth_schema.md`](./schema/auth_schema.md)
- [`schema/problem_schema.md`](./schema/problem_schema.md)
- [`schema/submission_schema.md`](./schema/submission_schema.md)
- [`schema/user_problem_schema.md`](./schema/user_problem_schema.md)
