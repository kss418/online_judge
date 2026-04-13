#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
backend_root="$(cd "${script_dir}/.." && pwd)"
repo_root="$(cd "${backend_root}/.." && pwd)"
active_settings_sql_file="${script_dir}/sql/check_db_performance_debugging_active_settings.sql"
extension_sql_file="${script_dir}/sql/check_db_performance_debugging_extension.sql"
file_settings_sql_file="${script_dir}/sql/check_db_performance_debugging_file_settings.sql"

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

if ! resolve_read_database_url "${db_name}" >/dev/null; then
    exit 1
fi

echo "== active settings =="
psql_read_run -P pager=off -f "${active_settings_sql_file}"

echo
echo "== extension =="
psql_read_run -P pager=off -f "${extension_sql_file}"

if [[ -n "${admin_user}" && -n "${admin_password}" ]]; then
    admin_error_file="$(mktemp)"
    trap 'rm -f "${admin_error_file}"' EXIT

    if psql_admin_run -c 'SELECT 1;' >/dev/null 2>"${admin_error_file}"; then
        echo
        echo "== configured file settings =="
        psql_admin_run -P pager=off -f "${file_settings_sql_file}"
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
