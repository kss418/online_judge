#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
backend_root="$(cd "${script_dir}/.." && pwd)"
repo_root="$(cd "${backend_root}/.." && pwd)"
shared_preload_sql_file="${script_dir}/sql/enable_db_performance_debugging_shared_preload.sql"
alter_system_sql_file="${script_dir}/sql/enable_db_performance_debugging_alter_system.sql"
pg_stat_statements_sql_file="${script_dir}/sql/enable_db_performance_debugging_pg_stat_statements.sql"
create_extension_sql_file="${script_dir}/sql/enable_db_performance_debugging_create_extension.sql"

# shellcheck disable=SC1091
source "${repo_root}/scripts/lib/postgres.sh"

source_project_env "${backend_root}"
require_psql

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

if ! psql_admin_run -c 'SELECT 1;' >/dev/null 2>"${connect_error_file}"; then
    cat "${connect_error_file}" >&2
    echo "hint: enable_db_performance_debugging.sh needs a superuser-capable DB_ADMIN_USER/DB_ADMIN_PASSWORD" >&2
    exit 1
fi

shared_preload_before="$(
    psql_admin_run -qAt -f "${shared_preload_sql_file}"
)"

if [[ -z "${shared_preload_before}" ]]; then
    desired_shared_preload="pg_stat_statements"
elif [[ "${shared_preload_before}" == *"pg_stat_statements"* ]]; then
    desired_shared_preload="${shared_preload_before}"
else
    desired_shared_preload="${shared_preload_before},pg_stat_statements"
fi

psql_admin_run \
    -v desired_shared_preload="${desired_shared_preload}" \
    -f "${alter_system_sql_file}"

if [[ "${shared_preload_before}" == *"pg_stat_statements"* ]]; then
    psql_admin_run -f "${pg_stat_statements_sql_file}"
fi

psql_admin_run -c "SELECT pg_reload_conf();"

if [[ "${shared_preload_before}" == *"pg_stat_statements"* ]]; then
    psql_admin_run "${db_name}" -f "${create_extension_sql_file}"

    echo "configured db performance debugging settings"
    echo "pg_stat_statements extension is ready in database: ${db_name}"
else
    echo "configured db performance debugging settings"
    echo "postgres restart is still required before pg_stat_statements becomes active"
    echo "after restarting postgres, rerun this script once to create the extension if needed"
fi

echo "run: bash ./scripts/check_db_performance_debugging.sh"
