#!/usr/bin/env bash

postgres_lib_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
source "${postgres_lib_dir}/env.sh"

source_project_env(){
    local backend_root="${1:-${project_root:-${backend_root:-}}}"
    local env_file=""
    local env_example_file=""

    if [[ -z "${backend_root}" ]]; then
        echo "error: backend root is required to load project env" >&2
        return 1
    fi

    env_file="${backend_root}/.env"
    env_example_file="${backend_root}/.env.example"

    if [[ -f "${env_file}" ]]; then
        load_env_file "${env_file}" "${env_example_file}"
    fi
}

require_psql(){
    require_command psql
}

build_postgres_url(){
    local db_user="$1"
    local db_password="$2"
    local db_host="$3"
    local db_port="$4"
    local db_name="$5"

    printf 'postgresql://%s:%s@%s:%s/%s\n' \
        "${db_user}" \
        "${db_password}" \
        "${db_host}" \
        "${db_port}" \
        "${db_name}"
}

resolve_database_url(){
    local database_url="${DATABASE_URL:-}"
    local db_user=""
    local db_password=""
    local db_host=""
    local db_port=""
    local db_name=""

    if [[ -n "${database_url}" ]]; then
        printf '%s\n' "${database_url}"
        return 0
    fi

    db_user="${DB_USER:-}"
    db_password="${DB_PASSWORD:-}"
    db_host="${DB_HOST:-}"
    db_port="${DB_PORT:-5432}"
    db_name="${DB_NAME:-}"

    if [[ -z "${db_user}" || -z "${db_password}" || -z "${db_host}" || -z "${db_name}" ]]; then
        echo "error: DATABASE_URL is empty" >&2
        echo "hint: set DATABASE_URL or DB_USER/DB_PASSWORD/DB_HOST/DB_PORT/DB_NAME in .env" >&2
        return 1
    fi

    build_postgres_url "${db_user}" "${db_password}" "${db_host}" "${db_port}" "${db_name}"
}

resolve_admin_database_url(){
    local database_name="${1:-postgres}"
    local db_host="${DB_HOST:-localhost}"
    local db_port="${DB_PORT:-5432}"
    local admin_user="${DB_ADMIN_USER:-}"
    local admin_password="${DB_ADMIN_PASSWORD:-}"

    if [[ -z "${admin_user}" || -z "${admin_password}" ]]; then
        echo "error: DB_ADMIN_USER/DB_ADMIN_PASSWORD must be set in .env" >&2
        return 1
    fi

    build_postgres_url "${admin_user}" "${admin_password}" "${db_host}" "${db_port}" "${database_name}"
}

resolve_read_database_url(){
    local database_name="${1:-${DB_NAME:-}}"
    local db_host="${DB_HOST:-localhost}"
    local db_port="${DB_PORT:-5432}"
    local read_user=""
    local read_password=""

    if [[ -z "${database_name}" ]]; then
        echo "error: DB_NAME must be set in .env" >&2
        return 1
    fi

    if [[ -n "${DB_USER:-}" || -n "${DB_PASSWORD:-}" ]]; then
        if [[ -z "${DB_USER:-}" || -z "${DB_PASSWORD:-}" ]]; then
            echo "error: DB_USER/DB_PASSWORD or DB_ADMIN_USER/DB_ADMIN_PASSWORD must be set in .env" >&2
            return 1
        fi

        read_user="${DB_USER}"
        read_password="${DB_PASSWORD}"
    elif [[ -n "${DB_ADMIN_USER:-}" || -n "${DB_ADMIN_PASSWORD:-}" ]]; then
        if [[ -z "${DB_ADMIN_USER:-}" || -z "${DB_ADMIN_PASSWORD:-}" ]]; then
            echo "error: DB_USER/DB_PASSWORD or DB_ADMIN_USER/DB_ADMIN_PASSWORD must be set in .env" >&2
            return 1
        fi

        read_user="${DB_ADMIN_USER}"
        read_password="${DB_ADMIN_PASSWORD}"
    else
        echo "error: DB_USER/DB_PASSWORD or DB_ADMIN_USER/DB_ADMIN_PASSWORD must be set in .env" >&2
        return 1
    fi

    build_postgres_url "${read_user}" "${read_password}" "${db_host}" "${db_port}" "${database_name}"
}

psql_url_run(){
    local database_url="$1"
    shift

    psql "${database_url}" -X -v ON_ERROR_STOP=1 "$@"
}

psql_run(){
    local database_url=""

    database_url="$(resolve_database_url)" || return 1
    psql_url_run "${database_url}" "$@"
}

psql_admin_run(){
    local database_name="postgres"
    local database_url=""

    if (($# > 0)) && [[ "$1" != -* ]]; then
        database_name="$1"
        shift
    fi

    database_url="$(resolve_admin_database_url "${database_name}")" || return 1
    psql_url_run "${database_url}" "$@"
}

psql_read_run(){
    local database_name="${DB_NAME:-}"
    local database_url=""

    if (($# > 0)) && [[ "$1" != -* ]]; then
        database_name="$1"
        shift
    fi

    database_url="$(resolve_read_database_url "${database_name}")" || return 1
    psql_url_run "${database_url}" "$@"
}
