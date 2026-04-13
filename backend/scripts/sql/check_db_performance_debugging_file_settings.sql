SELECT
    name,
    setting,
    applied,
    error
FROM pg_file_settings
WHERE name IN (
    'shared_preload_libraries',
    'compute_query_id',
    'log_lock_waits',
    'deadlock_timeout',
    'pg_stat_statements.max',
    'pg_stat_statements.track',
    'pg_stat_statements.save'
)
ORDER BY name;
