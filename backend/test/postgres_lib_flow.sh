#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
backend_root="$(cd "${script_dir}/.." && pwd)"
repo_root="$(cd "${backend_root}/.." && pwd)"

# shellcheck disable=SC1091
source "${repo_root}/scripts/lib/postgres.sh"

original_path="${PATH}"
tmp_dir="$(mktemp -d /tmp/postgres_lib_flow.XXXXXX)"
fake_bin_dir="${tmp_dir}/bin"
fake_psql_log="${tmp_dir}/fake_psql.log"

db_env_vars=(
    DATABASE_URL
    DB_USER
    DB_PASSWORD
    DB_HOST
    DB_PORT
    DB_NAME
    DB_ADMIN_USER
    DB_ADMIN_PASSWORD
    DDL_USER
    DDL_PASSWORD
)

cleanup(){
    local env_var=""

    PATH="${original_path}"
    for env_var in "${db_env_vars[@]}"; do
        unset "${env_var}" || true
    done
    rm -rf "${tmp_dir}"
}

fail(){
    echo "error: $*" >&2
    exit 1
}

assert_eq(){
    local actual="$1"
    local expected="$2"
    local message="$3"

    if [[ "${actual}" != "${expected}" ]]; then
        fail "${message}: expected='${expected}' actual='${actual}'"
    fi
}

assert_contains(){
    local actual="$1"
    local expected_fragment="$2"
    local message="$3"

    if [[ "${actual}" != *"${expected_fragment}"* ]]; then
        fail "${message}: missing='${expected_fragment}' actual='${actual}'"
    fi
}

reset_db_env(){
    local env_var=""

    for env_var in "${db_env_vars[@]}"; do
        unset "${env_var}" || true
    done
}

capture_failure(){
    local output=""
    local status=0

    set +e
    output="$("$@" 2>&1)"
    status=$?
    set -e

    if (( status == 0 )); then
        fail "expected command failure: $*"
    fi

    printf '%s\n' "${output}"
}

write_fake_psql(){
    mkdir -p "${fake_bin_dir}"
    cat >"${fake_bin_dir}/psql" <<'EOF'
#!/usr/bin/env bash
printf '%s\n' "$@" >"${FAKE_PSQL_LOG}"
EOF
    chmod +x "${fake_bin_dir}/psql"
    export FAKE_PSQL_LOG="${fake_psql_log}"
    PATH="${fake_bin_dir}:${PATH}"
}

assert_logged_args(){
    local expected_url="$1"
    local expected_tail="$2"
    local actual_log=""

    actual_log="$(tr '\n' ' ' <"${fake_psql_log}")"
    assert_contains "${actual_log}" "${expected_url}" "psql URL mismatch"
    assert_contains "${actual_log}" "-X -v ON_ERROR_STOP=1" "psql bootstrap flags missing"
    assert_contains "${actual_log}" "${expected_tail}" "psql forwarded args missing"
}

trap cleanup EXIT

reset_db_env
temp_backend_root="${tmp_dir}/backend"
mkdir -p "${temp_backend_root}"
printf '%s\n' \
    "DB_USER=env_user" \
    "DB_PASSWORD=env_password" \
    "DB_HOST=env_host" \
    "DB_PORT=15432" \
    "DB_NAME=env_db" \
    >"${temp_backend_root}/.env"
printf '%s\n' "# test env example" >"${temp_backend_root}/.env.example"

source_project_env "${temp_backend_root}"
assert_eq "${DB_USER}" "env_user" "source_project_env should load DB_USER"
assert_eq "${DB_PASSWORD}" "env_password" "source_project_env should load DB_PASSWORD"
assert_eq "${DB_HOST}" "env_host" "source_project_env should load DB_HOST"
assert_eq "${DB_PORT}" "15432" "source_project_env should load DB_PORT"
assert_eq "${DB_NAME}" "env_db" "source_project_env should load DB_NAME"

reset_db_env
DATABASE_URL="postgresql://override:override@db.example:5432/override_db"
DB_USER="ignored_user"
DB_PASSWORD="ignored_password"
DB_HOST="ignored_host"
DB_PORT="9999"
DB_NAME="ignored_db"
assert_eq \
    "$(resolve_database_url)" \
    "postgresql://override:override@db.example:5432/override_db" \
    "resolve_database_url should prefer DATABASE_URL"

reset_db_env
DB_USER="app_user"
DB_PASSWORD="app_password"
DB_HOST="db.internal"
DB_PORT="6543"
DB_NAME="judge_db"
assert_eq \
    "$(resolve_database_url)" \
    "postgresql://app_user:app_password@db.internal:6543/judge_db" \
    "resolve_database_url should build URL from DB_*"

reset_db_env
DB_HOST="db.internal"
DB_PORT="6543"
DB_NAME="judge_db"
DB_ADMIN_USER="admin_user"
DB_ADMIN_PASSWORD="admin_password"
assert_eq \
    "$(resolve_admin_database_url)" \
    "postgresql://admin_user:admin_password@db.internal:6543/postgres" \
    "resolve_admin_database_url should default to postgres"
assert_eq \
    "$(resolve_read_database_url)" \
    "postgresql://admin_user:admin_password@db.internal:6543/judge_db" \
    "resolve_read_database_url should fall back to admin credentials"

DB_USER="read_user"
DB_PASSWORD="read_password"
assert_eq \
    "$(resolve_read_database_url)" \
    "postgresql://read_user:read_password@db.internal:6543/judge_db" \
    "resolve_read_database_url should prefer DB_USER/DB_PASSWORD"

reset_db_env
assert_contains \
    "$(capture_failure resolve_database_url)" \
    "error: DATABASE_URL is empty" \
    "resolve_database_url should explain missing env"

reset_db_env
DB_NAME="judge_db"
DB_USER="broken_read_user"
assert_contains \
    "$(capture_failure resolve_read_database_url)" \
    "error: DB_USER/DB_PASSWORD or DB_ADMIN_USER/DB_ADMIN_PASSWORD must be set in .env" \
    "resolve_read_database_url should reject partial credentials"

write_fake_psql
require_psql

reset_db_env
DB_USER="app_user"
DB_PASSWORD="app_password"
DB_HOST="db.internal"
DB_PORT="6543"
DB_NAME="judge_db"
psql_run -qAt -f /tmp/query.sql >/dev/null
assert_logged_args \
    "postgresql://app_user:app_password@db.internal:6543/judge_db" \
    "-qAt -f /tmp/query.sql"

reset_db_env
DB_HOST="db.internal"
DB_PORT="6543"
DB_NAME="judge_db"
DB_ADMIN_USER="admin_user"
DB_ADMIN_PASSWORD="admin_password"
psql_admin_run judge_db -c "SELECT 1;" >/dev/null
assert_logged_args \
    "postgresql://admin_user:admin_password@db.internal:6543/judge_db" \
    "-c SELECT 1;"

psql_read_run -P pager=off -f /tmp/read_query.sql >/dev/null
assert_logged_args \
    "postgresql://admin_user:admin_password@db.internal:6543/judge_db" \
    "-P pager=off -f /tmp/read_query.sql"

echo "postgres_lib_flow passed"
