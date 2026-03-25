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

admin_database_url="postgresql://${admin_user}:${admin_password}@${db_host_for_output}:${db_port_for_output}/postgres"
target_admin_database_url="postgresql://${admin_user}:${admin_password}@${db_host_for_output}:${db_port_for_output}/${db_name_for_output}"

psql "${admin_database_url}" \
    -v ON_ERROR_STOP=1 \
    -v ddl_user="${ddl_user}" \
    -v ddl_password="${ddl_password}" \
    -v db_name="${db_name}" <<'SQL'
SELECT format('CREATE ROLE %I WITH LOGIN PASSWORD %L', :'ddl_user', :'ddl_password')
WHERE NOT EXISTS(SELECT 1 FROM pg_roles WHERE rolname = :'ddl_user')
\gexec

SELECT format('ALTER ROLE %I WITH LOGIN PASSWORD %L', :'ddl_user', :'ddl_password')
\gexec

SELECT format('CREATE DATABASE %I', :'db_name')
WHERE NOT EXISTS(SELECT 1 FROM pg_database WHERE datname = :'db_name')
\gexec

GRANT CONNECT ON DATABASE :"db_name" TO :"ddl_user";
SQL

psql "${target_admin_database_url}" \
    -v ON_ERROR_STOP=1 \
    -v ddl_user="${ddl_user}" <<'SQL'
GRANT USAGE, CREATE ON SCHEMA public TO :"ddl_user";
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO :"ddl_user";
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO :"ddl_user";
GRANT ALL PRIVILEGES ON ALL FUNCTIONS IN SCHEMA public TO :"ddl_user";
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON TABLES TO :"ddl_user";
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON SEQUENCES TO :"ddl_user";
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON FUNCTIONS TO :"ddl_user";
SQL

ddl_database_url="postgresql://${ddl_user}:${ddl_password}@${db_host_for_output}:${db_port_for_output}/${db_name_for_output}"
echo "created ddl role: ${ddl_user}"
echo "DDL_DATABASE_URL=${ddl_database_url}"
