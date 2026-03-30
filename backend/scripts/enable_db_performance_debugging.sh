#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"
env_file="${project_root}/.env"

if [[ -f "${env_file}" ]]; then
    set -a
    # shellcheck disable=SC1090
    source "${env_file}"
    set +a
fi

if ! command -v psql >/dev/null 2>&1; then
    echo "error: psql command not found" >&2
    exit 1
fi

db_host="${DB_HOST:-localhost}"
db_port="${DB_PORT:-5432}"
db_name="${DB_NAME:-}"
admin_user="${DB_ADMIN_USER:-}"
admin_password="${DB_ADMIN_PASSWORD:-}"

if [[ -z "${db_name}" ]]; then
    echo "error: DB_NAME must be set in .env" >&2
    exit 1
fi

if [[ -z "${admin_user}" || -z "${admin_password}" ]]; then
    echo "error: DB_ADMIN_USER/DB_ADMIN_PASSWORD must be set in .env" >&2
    exit 1
fi

connect_error_file="$(mktemp)"
trap 'rm -f "${connect_error_file}"' EXIT

if ! PGPASSWORD="${admin_password}" psql \
    -X \
    -h "${db_host}" \
    -p "${db_port}" \
    -U "${admin_user}" \
    -d postgres \
    -v ON_ERROR_STOP=1 \
    -c 'SELECT 1;' >/dev/null 2>"${connect_error_file}"; then
    cat "${connect_error_file}" >&2
    echo "hint: enable_db_performance_debugging.sh needs a superuser-capable DB_ADMIN_USER/DB_ADMIN_PASSWORD" >&2
    exit 1
fi

configure_sql="$(cat <<'SQL'
DO $do$
DECLARE
    preload_value TEXT := current_setting('shared_preload_libraries', true);
BEGIN
    IF preload_value IS NULL OR btrim(preload_value) = '' THEN
        EXECUTE 'ALTER SYSTEM SET shared_preload_libraries = ''pg_stat_statements''';
    ELSIF position('pg_stat_statements' IN preload_value) = 0 THEN
        EXECUTE format(
            'ALTER SYSTEM SET shared_preload_libraries = %L',
            preload_value || ',pg_stat_statements'
        );
    END IF;
END
$do$;

ALTER SYSTEM SET compute_query_id = 'on';
ALTER SYSTEM SET pg_stat_statements.max = '10000';
ALTER SYSTEM SET pg_stat_statements.track = 'top';
ALTER SYSTEM SET pg_stat_statements.save = 'on';
ALTER SYSTEM SET log_lock_waits = 'on';
ALTER SYSTEM SET deadlock_timeout = '50ms';

SELECT pg_reload_conf();
SQL
)"

shared_preload_before="$(
    PGPASSWORD="${admin_password}" psql \
        -X \
        -A \
        -t \
        -h "${db_host}" \
        -p "${db_port}" \
        -U "${admin_user}" \
        -d postgres \
        -v ON_ERROR_STOP=1 \
        -c "SELECT current_setting('shared_preload_libraries', true);"
)"

PGPASSWORD="${admin_password}" psql \
    -X \
    -h "${db_host}" \
    -p "${db_port}" \
    -U "${admin_user}" \
    -d postgres \
    -v ON_ERROR_STOP=1 \
    -c "${configure_sql}"

if [[ "${shared_preload_before}" == *"pg_stat_statements"* ]]; then
    PGPASSWORD="${admin_password}" psql \
        -X \
        -h "${db_host}" \
        -p "${db_port}" \
        -U "${admin_user}" \
        -d "${db_name}" \
        -v ON_ERROR_STOP=1 \
        -c "CREATE EXTENSION IF NOT EXISTS pg_stat_statements;"

    echo "configured db performance debugging settings"
    echo "pg_stat_statements extension is ready in database: ${db_name}"
else
    echo "configured db performance debugging settings"
    echo "postgres restart is still required before pg_stat_statements becomes active"
    echo "after restarting postgres, rerun this script once to create the extension if needed"
fi

echo "run: bash ./scripts/check_db_performance_debugging.sh"
