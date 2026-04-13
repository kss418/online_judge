SELECT extname, extversion
FROM pg_extension
WHERE extname = 'pg_stat_statements';
