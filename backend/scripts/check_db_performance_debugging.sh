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
read_user="${DB_USER:-${DB_ADMIN_USER:-}}"
read_password="${DB_PASSWORD:-${DB_ADMIN_PASSWORD:-}}"
admin_user="${DB_ADMIN_USER:-}"
admin_password="${DB_ADMIN_PASSWORD:-}"

if [[ -z "${db_name}" ]]; then
    echo "error: DB_NAME must be set in .env" >&2
    exit 1
fi

if [[ -z "${read_user}" || -z "${read_password}" ]]; then
    echo "error: DB_USER/DB_PASSWORD or DB_ADMIN_USER/DB_ADMIN_PASSWORD must be set in .env" >&2
    exit 1
fi

active_settings_sql="$(cat <<'SQL'
SELECT
    name,
    setting,
    unit,
    pending_restart
FROM pg_settings
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
SQL
)"

extension_sql="$(cat <<'SQL'
SELECT extname, extversion
FROM pg_extension
WHERE extname = 'pg_stat_statements';
SQL
)"

file_settings_sql="$(cat <<'SQL'
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
SQL
)"

echo "== active settings =="
PGPASSWORD="${read_password}" psql \
    -X \
    -h "${db_host}" \
    -p "${db_port}" \
    -U "${read_user}" \
    -d "${db_name}" \
    -P pager=off \
    -v ON_ERROR_STOP=1 \
    -c "${active_settings_sql}"

echo
echo "== extension =="
PGPASSWORD="${read_password}" psql \
    -X \
    -h "${db_host}" \
    -p "${db_port}" \
    -U "${read_user}" \
    -d "${db_name}" \
    -P pager=off \
    -v ON_ERROR_STOP=1 \
    -c "${extension_sql}"

if [[ -n "${admin_user}" && -n "${admin_password}" ]]; then
    admin_error_file="$(mktemp)"
    trap 'rm -f "${admin_error_file}"' EXIT

    if PGPASSWORD="${admin_password}" psql \
        -X \
        -h "${db_host}" \
        -p "${db_port}" \
        -U "${admin_user}" \
        -d postgres \
        -v ON_ERROR_STOP=1 \
        -c 'SELECT 1;' >/dev/null 2>"${admin_error_file}"; then
        echo
        echo "== configured file settings =="
        PGPASSWORD="${admin_password}" psql \
            -X \
            -h "${db_host}" \
            -p "${db_port}" \
            -U "${admin_user}" \
            -d postgres \
            -P pager=off \
            -v ON_ERROR_STOP=1 \
            -c "${file_settings_sql}"
    else
        echo
        echo "== configured file settings =="
        echo "skipped: DB_ADMIN_USER/DB_ADMIN_PASSWORD could not connect to postgres"
    fi

    rm -f "${admin_error_file}"
    trap - EXIT
else
    echo
    echo "== configured file settings =="
    echo "skipped: DB_ADMIN_USER/DB_ADMIN_PASSWORD are not set"
fi
