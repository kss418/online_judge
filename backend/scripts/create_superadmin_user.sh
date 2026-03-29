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

compute_sha512_hex(){
    local raw_value="${1:-}"

    if command -v sha512sum >/dev/null 2>&1; then
        printf '%s' "${raw_value}" | sha512sum | awk '{print $1}'
        return
    fi

    if command -v shasum >/dev/null 2>&1; then
        printf '%s' "${raw_value}" | shasum -a 512 | awk '{print $1}'
        return
    fi

    if command -v openssl >/dev/null 2>&1; then
        printf '%s' "${raw_value}" | openssl dgst -sha512 -r | awk '{print $1}'
        return
    fi

    echo "error: sha512 hashing command not found (need sha512sum, shasum, or openssl)" >&2
    exit 1
}

validate_credential_length(){
    local field_name="$1"
    local field_value="$2"
    local field_length=${#field_value}

    if (( field_length < 4 || field_length > 15 )); then
        echo "error: ${field_name} length must be between 4 and 15 characters" >&2
        exit 1
    fi
}

superadmin_login_id="${1:-admin}"
superadmin_password="${2:-admin}"
superadmin_password_hash="$(compute_sha512_hex "${superadmin_password}")"

validate_credential_length "id" "${superadmin_login_id}"
validate_credential_length "password" "${superadmin_password}"

database_url="${DATABASE_URL:-}"
if [[ -z "${database_url}" ]]; then
    db_user="${DB_USER:-}"
    db_password="${DB_PASSWORD:-}"
    db_host="${DB_HOST:-}"
    db_port="${DB_PORT:-5432}"
    db_name="${DB_NAME:-}"

    if [[ -z "${db_user}" || -z "${db_password}" || -z "${db_host}" || -z "${db_name}" ]]; then
        echo "error: DATABASE_URL is empty" >&2
        echo "hint: set DATABASE_URL or DB_USER/DB_PASSWORD/DB_HOST/DB_PORT/DB_NAME in .env" >&2
        exit 1
    fi

    database_url="postgresql://${db_user}:${db_password}@${db_host}:${db_port}/${db_name}"
fi

echo "ensure superadmin user"
existing_user_id="$(
    psql "${database_url}" \
        -v ON_ERROR_STOP=1 \
        -v superadmin_login_id="${superadmin_login_id}" \
        -qAt <<'SQL'
SELECT user_id
FROM users
WHERE user_login_id = :'superadmin_login_id'
LIMIT 1;
SQL
)"

if [[ -n "${existing_user_id}" && ! "${existing_user_id}" =~ ^-?[0-9]+$ ]]; then
    echo "error: invalid existing user_id: ${existing_user_id}" >&2
    exit 1
fi

if [[ -z "${existing_user_id}" ]]; then
    psql "${database_url}" \
        -v ON_ERROR_STOP=1 \
        -v superadmin_login_id="${superadmin_login_id}" \
        -v superadmin_password_hash="${superadmin_password_hash}" <<'SQL'
BEGIN;

WITH new_user_info AS (
    INSERT INTO user_info DEFAULT VALUES
    RETURNING user_id
)
INSERT INTO users(
    user_id,
    user_login_id,
    user_password_hash,
    permission_level
)
SELECT
    user_id,
    :'superadmin_login_id',
    :'superadmin_password_hash',
    2
FROM new_user_info;

COMMIT;
SQL
elif (( existing_user_id <= 0 )); then
    psql "${database_url}" \
        -v ON_ERROR_STOP=1 \
        -v existing_user_id="${existing_user_id}" \
        -v superadmin_login_id="${superadmin_login_id}" \
        -v superadmin_password_hash="${superadmin_password_hash}" <<'SQL'
BEGIN;

UPDATE users
SET
    user_login_id = user_login_id || '__legacy_' || txid_current()::text,
    user_password_hash = NULL,
    permission_level = 0,
    auth_updated_at = NOW()
WHERE user_id = :'existing_user_id';

UPDATE user_info
SET updated_at = NOW()
WHERE user_id = :'existing_user_id';

WITH new_user_info AS (
    INSERT INTO user_info DEFAULT VALUES
    RETURNING user_id
)
INSERT INTO users(
    user_id,
    user_login_id,
    user_password_hash,
    permission_level
)
SELECT
    user_id,
    :'superadmin_login_id',
    :'superadmin_password_hash',
    2
FROM new_user_info;

COMMIT;
SQL
else
    psql "${database_url}" \
        -v ON_ERROR_STOP=1 \
        -v existing_user_id="${existing_user_id}" \
        -v superadmin_password_hash="${superadmin_password_hash}" <<'SQL'
BEGIN;

INSERT INTO user_info(user_id)
VALUES(:'existing_user_id')
ON CONFLICT(user_id) DO UPDATE
SET updated_at = NOW();

UPDATE users
SET
    user_password_hash = :'superadmin_password_hash',
    permission_level = 2,
    auth_updated_at = NOW()
WHERE user_id = :'existing_user_id';

COMMIT;
SQL
fi

ensured_user_id="$(
    psql "${database_url}" \
        -v ON_ERROR_STOP=1 \
        -v superadmin_login_id="${superadmin_login_id}" \
        -qAt <<'SQL'
SELECT user_id
FROM users
WHERE user_login_id = :'superadmin_login_id'
LIMIT 1;
SQL
)"

echo "superadmin user ensured: user_id=${ensured_user_id}, user_login_id=${superadmin_login_id}"
