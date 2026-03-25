#!/usr/bin/env bash

require_db_env(){
    if [[ -z "${DB_HOST:-}" || -z "${DB_PORT:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWORD:-}" || -z "${DB_NAME:-}" ]]; then
        echo "missing required db envs" >&2
        exit 1
    fi
}

require_db_admin_env(){
    if [[ -z "${DB_ADMIN_USER:-}" || -z "${DB_ADMIN_PASSWORD:-}" ]]; then
        echo "missing required db admin envs" >&2
        exit 1
    fi
}

require_database_test_env(){
    local missing_vars=()

    [[ -n "${project_root:-}" ]] || missing_vars+=("project_root")
    [[ -n "${test_db_name:-}" ]] || missing_vars+=("test_db_name")
    [[ -n "${test_database_url:-}" ]] || missing_vars+=("test_database_url")
    [[ -n "${test_log_temp_file:-}" ]] || missing_vars+=("test_log_temp_file")

    if (( ${#missing_vars[@]} > 0 )); then
        printf 'missing required database test vars: %s\n' "${missing_vars[*]}" >&2
        return 1
    fi
}

publish_database_failure_logs(){
    if [[ "$(type -t publish_all_failure_logs || true)" == "function" ]]; then
        publish_all_failure_logs
        return 0
    fi

    if [[ "$(type -t publish_failure_logs || true)" == "function" ]]; then
        publish_failure_logs
    fi
}

create_test_database(){
    require_db_env
    require_db_admin_env

    if [[ -z "${test_db_name:-}" ]]; then
        echo "missing test_db_name" >&2
        return 1
    fi

    PGPASSWORD="${DB_ADMIN_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_ADMIN_USER}" \
        -d postgres \
        -v ON_ERROR_STOP=1 <<SQL >/dev/null
CREATE DATABASE "${test_db_name}" OWNER "${DB_USER}";
SQL

    test_database_created="1"
}

drop_test_database(){
    if [[ "${test_database_created:-0}" != "1" ]]; then
        return 0
    fi

    if [[ -z "${DB_HOST:-}" || -z "${DB_PORT:-}" || -z "${DB_ADMIN_USER:-}" || -z "${DB_ADMIN_PASSWORD:-}" || -z "${test_db_name:-}" ]]; then
        return 0
    fi

    PGPASSWORD="${DB_ADMIN_PASSWORD}" psql \
        -X \
        -h "${DB_HOST}" \
        -p "${DB_PORT}" \
        -U "${DB_ADMIN_USER}" \
        -d postgres \
        -v ON_ERROR_STOP=1 <<SQL >/dev/null 2>&1 || true
SELECT pg_terminate_backend(pid)
FROM pg_stat_activity
WHERE datname = '${test_db_name}' AND pid <> pg_backend_pid();

DROP DATABASE IF EXISTS "${test_db_name}";
SQL
}

run_schema_migration(){
    local migration_script_path="$1"
    local migration_name="$2"

    require_database_test_env

    if ! DATABASE_URL="${test_database_url}" bash "${migration_script_path}" >>"${test_log_temp_file}" 2>&1; then
        append_log_line "${test_log_temp_file}" "migration failed: ${migration_name}"
        publish_database_failure_logs
        echo "failed to apply ${migration_name}" >&2
        return 1
    fi

    append_log_line "${test_log_temp_file}" "migration applied: ${migration_name}"
}

apply_test_database_migrations(){
    require_database_test_env

    run_schema_migration "${project_root}/scripts/migrate_auth_schema.sh" "auth_schema"
    run_schema_migration "${project_root}/scripts/migrate_problem_schema.sh" "problem_schema"
    run_schema_migration "${project_root}/scripts/migrate_submission_schema.sh" "submission_schema"
}
