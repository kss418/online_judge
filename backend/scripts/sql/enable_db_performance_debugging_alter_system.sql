ALTER SYSTEM SET shared_preload_libraries = :'desired_shared_preload';
ALTER SYSTEM SET compute_query_id = 'on';
ALTER SYSTEM SET log_lock_waits = 'on';
ALTER SYSTEM SET deadlock_timeout = '50ms';
