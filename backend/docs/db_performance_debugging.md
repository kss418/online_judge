# DB Performance Debugging

This project includes helper scripts for enabling and checking the PostgreSQL
settings that are most useful when `submission create` looks slower than the
judge-side timings suggest.

## What gets enabled

- `pg_stat_statements`
  - Finds the SQL statements with the highest total or average execution time.
- `log_lock_waits = on`
  - Writes a PostgreSQL log entry when a statement waits on a lock longer than
    `deadlock_timeout`.
- `deadlock_timeout = 50ms`
  - Lowers the wait threshold enough to surface row-lock hot spots during load
    testing.
- `compute_query_id = on`
  - Lets PostgreSQL normalize and aggregate SQL in `pg_stat_statements`.

## Scripts

- Enable or update the settings:
  - `bash ./scripts/enable_db_performance_debugging.sh`
- Check active values, configured file values, and extension status:
  - `bash ./scripts/check_db_performance_debugging.sh`

Both scripts load `backend/.env`.

- SQL for these helpers lives under `scripts/sql/`
  - `enable_db_performance_debugging_*.sql`
  - `check_db_performance_debugging_*.sql`

- `enable_db_performance_debugging.sh` requires `DB_ADMIN_USER` /
  `DB_ADMIN_PASSWORD`.
- `check_db_performance_debugging.sh` reads active settings with
  `DB_USER` / `DB_PASSWORD` when available, and only uses
  `DB_ADMIN_USER` / `DB_ADMIN_PASSWORD` for the optional
  `pg_file_settings` view.

## Restart note

`shared_preload_libraries` only becomes active after PostgreSQL restarts. The
enable script updates the setting immediately, but if `pg_stat_statements` was
not already preloaded, the script will stop short of creating the extension and
will ask you to restart PostgreSQL first.

## Suggested workflow

1. Run `bash ./scripts/enable_db_performance_debugging.sh`.
2. If the script says restart is needed, restart PostgreSQL.
3. Run `bash ./scripts/enable_db_performance_debugging.sh` again.
4. Verify with `bash ./scripts/check_db_performance_debugging.sh`.
5. Reset statement stats before a focused load test:
   - `PGPASSWORD="$DB_ADMIN_PASSWORD" psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_ADMIN_USER" -d "$DB_NAME" -c "SELECT pg_stat_statements_reset();"`
6. Run the load test.
7. Inspect the hottest statements:

```sql
SELECT
    calls,
    total_exec_time,
    mean_exec_time,
    rows,
    query
FROM pg_stat_statements
ORDER BY total_exec_time DESC
LIMIT 20;
```

```sql
SELECT
    calls,
    total_exec_time,
    mean_exec_time,
    rows,
    query
FROM pg_stat_statements
WHERE calls > 10
ORDER BY mean_exec_time DESC
LIMIT 20;
```

## What to look for in this codebase

These repository queries are the most likely lock or hot-row candidates during
burst submission traffic.

- `user_info ... FOR UPDATE` in
  [`src/db_repository/user_repository.cpp`](../src/db_repository/user_repository.cpp)
- `INSERT ... ON CONFLICT DO UPDATE` on `user_problem_attempt_summary` in
  [`src/db_repository/user_problem_summary_repository.cpp`](../src/db_repository/user_problem_summary_repository.cpp)
- `UPDATE problem_statistics` in
  [`src/db_repository/problem_statistics_repository.cpp`](../src/db_repository/problem_statistics_repository.cpp)

If `pg_stat_statements` ranks these near the top and PostgreSQL also logs lock
waits, that is a strong signal that submission create latency is being driven
by row-lock contention rather than by HTTP handler overhead.
