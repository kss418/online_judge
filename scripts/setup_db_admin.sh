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

admin_user="${DB_ADMIN_USER:-postgres}"
admin_password="${DB_ADMIN_PASSWORD:-}"

if [[ -z "${admin_password}" ]]; then
    echo "error: DB_ADMIN_PASSWORD must be set in .env" >&2
    exit 1
fi

run_as_postgres(){
    if [[ "$(id -un)" == "postgres" ]]; then
        "$@"
        return
    fi

    if command -v sudo >/dev/null 2>&1; then
        sudo -u postgres "$@"
        return
    fi

    echo "error: this script must run as postgres user or have sudo available" >&2
    exit 1
}

run_as_postgres psql -d postgres \
    -v ON_ERROR_STOP=1 \
    -v admin_user="${admin_user}" \
    -v admin_password="${admin_password}" <<'SQL'
SELECT format('CREATE ROLE %I WITH LOGIN SUPERUSER PASSWORD %L', :'admin_user', :'admin_password')
WHERE NOT EXISTS(SELECT 1 FROM pg_roles WHERE rolname = :'admin_user')
\gexec

SELECT format('ALTER ROLE %I WITH LOGIN SUPERUSER PASSWORD %L', :'admin_user', :'admin_password')
\gexec
SQL

echo "configured db admin account: ${admin_user}"
