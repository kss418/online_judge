#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
backend_root="$(cd "${script_dir}/.." && pwd)"
repo_root="$(cd "${backend_root}/.." && pwd)"
admin_sql_file="${script_dir}/sql/create_ddl_role_admin.sql"
schema_grants_sql_file="${script_dir}/sql/create_ddl_role_schema_grants.sql"

# shellcheck disable=SC1091
source "${repo_root}/scripts/lib/postgres.sh"

source_project_env "${backend_root}"
require_psql

ddl_user="${DDL_USER:-${DB_USER:-}}"
ddl_password="${DDL_PASSWORD:-${DB_PASSWORD:-}}"
db_name="${DB_NAME:-}"
db_host_for_output="${DB_HOST:-localhost}"
db_port_for_output="${DB_PORT:-5432}"
db_name_for_output="${DB_NAME:-}"

if [[ -z "${ddl_user}" || -z "${ddl_password}" || -z "${db_name}" ]]; then
    echo "error: DB_USER/DB_PASSWORD/DB_NAME must be set in .env" >&2
    exit 1
fi

admin_user="${DB_ADMIN_USER:-}"
admin_password="${DB_ADMIN_PASSWORD:-}"

if [[ -z "${admin_user}" || -z "${admin_password}" ]]; then
    echo "error: DB_ADMIN_USER/DB_ADMIN_PASSWORD must be set in .env" >&2
    echo "hint: DB_USER/DB_PASSWORD are role-creation target values, not admin login" >&2
    exit 1
fi

psql_admin_run \
    -v ddl_user="${ddl_user}" \
    -v ddl_password="${ddl_password}" \
    -v db_name="${db_name}" \
    -f "${admin_sql_file}"

psql_admin_run "${db_name_for_output}" \
    -v ddl_user="${ddl_user}" \
    -f "${schema_grants_sql_file}"

ddl_database_url="$(build_postgres_url "${ddl_user}" "${ddl_password}" "${db_host_for_output}" "${db_port_for_output}" "${db_name_for_output}")"
echo "created ddl role: ${ddl_user}"
echo "DDL_DATABASE_URL=${ddl_database_url}"
