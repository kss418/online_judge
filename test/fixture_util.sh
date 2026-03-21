#!/usr/bin/env bash

require_fixture_test_env(){
    local missing_vars=()

    [[ -n "${base_url:-}" ]] || missing_vars+=("base_url")
    [[ -n "${test_log_temp_file:-}" ]] || missing_vars+=("test_log_temp_file")

    if (( ${#missing_vars[@]} > 0 )); then
        printf 'missing required fixture test vars: %s\n' "${missing_vars[*]}" >&2
        return 1
    fi
}

publish_fixture_failure_logs(){
    if [[ "$(type -t publish_all_failure_logs || true)" == "function" ]]; then
        publish_all_failure_logs
        return 0
    fi

    if [[ "$(type -t publish_failure_logs || true)" == "function" ]]; then
        publish_failure_logs
    fi
}

make_sign_up_request_body(){
    python3 - "$1" "$2" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "user_login_id": sys.argv[1],
            "raw_password": sys.argv[2],
        }
    )
)
PY
}

make_create_problem_request_body(){
    python3 - "$1" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "title": sys.argv[1],
        }
    )
)
PY
}

read_auth_response_user_id_and_token(){
    local response_file_path="$1"

    python3 - "${response_file_path}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

user_id = response.get("user_id")
token = response.get("token")
if not isinstance(user_id, int) or user_id <= 0:
    raise SystemExit("invalid user_id in auth response")
if not isinstance(token, str) or not token:
    raise SystemExit("invalid token in auth response")

print(user_id, token)
PY
}

sign_up_user(){
    local user_login_id="$1"
    local raw_password="$2"
    local response_file_path="$3"
    local failure_context="${4:-fixture}"
    local sign_up_request_body=""
    local sign_up_status_code=""

    require_fixture_test_env

    sign_up_request_body="$(make_sign_up_request_body "${user_login_id}" "${raw_password}")"
    sign_up_status_code="$(
        curl \
            --silent \
            --show-error \
            --output "${response_file_path}" \
            --write-out "%{http_code}" \
            -H "Content-Type: application/json" \
            -d "${sign_up_request_body}" \
            "${base_url}/api/auth/sign-up"
    )"

    if [[ "${sign_up_status_code}" != "201" ]]; then
        append_log_line "${test_log_temp_file}" "sign-up failed: status=${sign_up_status_code}"
        publish_fixture_failure_logs
        echo "${failure_context} sign-up failed: expected status 201, got ${sign_up_status_code}" >&2
        echo "response body:" >&2
        cat "${response_file_path}" >&2
        exit 1
    fi

    append_log_line "${test_log_temp_file}" "sign-up passed: status=${sign_up_status_code}"
    read_auth_response_user_id_and_token "${response_file_path}"
}

promote_admin_user(){
    local user_id="$1"
    local failure_context="${2:-fixture}"
    local promoted_user_id=""

    require_db_env

    if [[ -z "${user_id}" ]]; then
        echo "missing user_id" >&2
        exit 1
    fi

    promoted_user_id="$(
        PGPASSWORD="${DB_PASSWORD}" psql \
            -X \
            -h "${DB_HOST}" \
            -p "${DB_PORT}" \
            -U "${DB_USER}" \
            -d "${DB_NAME}" \
            -v ON_ERROR_STOP=1 \
            -qAt <<SQL | sed -n '1p'
UPDATE users
SET is_admin = TRUE, updated_at = NOW()
WHERE user_id = ${user_id}
RETURNING user_id;
SQL
    )"

    if [[ "${promoted_user_id}" != "${user_id}" ]]; then
        append_log_line "${test_log_temp_file}" "admin promotion failed: user_id=${user_id}"
        publish_fixture_failure_logs
        echo "${failure_context} admin promotion failed" >&2
        exit 1
    fi

    append_log_line "${test_log_temp_file}" "admin promotion succeeded: user_id=${user_id}"
    printf '%s\n' "${promoted_user_id}"
}

read_problem_id_from_create_problem_response(){
    local response_file_path="$1"

    python3 - "${response_file_path}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

problem_id = response.get("problem_id")
if not isinstance(problem_id, int) or problem_id <= 0:
    raise SystemExit("invalid problem_id in create problem response")

print(problem_id)
PY
}

create_problem_via_api(){
    local auth_token="$1"
    local response_file_path="$2"
    local failure_context="${3:-fixture}"
    local problem_title="${4:-fixture problem}"
    local create_problem_request_body=""
    local create_problem_status_code=""
    local problem_id=""

    require_fixture_test_env
    create_problem_request_body="$(make_create_problem_request_body "${problem_title}")"

    create_problem_status_code="$(
        curl \
            --silent \
            --show-error \
            --output "${response_file_path}" \
            --write-out "%{http_code}" \
            --request POST \
            -H "Authorization: Bearer ${auth_token}" \
            -H "Content-Type: application/json" \
            -d "${create_problem_request_body}" \
            "${base_url}/api/problem"
    )"

    if [[ "${create_problem_status_code}" != "201" ]]; then
        append_log_line "${test_log_temp_file}" "problem create failed: status=${create_problem_status_code}"
        publish_fixture_failure_logs
        echo "${failure_context} create problem failed: expected status 201, got ${create_problem_status_code}" >&2
        echo "response body:" >&2
        cat "${response_file_path}" >&2
        exit 1
    fi

    problem_id="$(read_problem_id_from_create_problem_response "${response_file_path}")"
    append_log_line \
        "${test_log_temp_file}" \
        "problem created: problem_id=${problem_id}, title=${problem_title}"
    printf '%s\n' "${problem_id}"
}

create_problem_in_db(){
    local failure_context="${1:-fixture}"
    local problem_title="${2:-fixture problem}"
    local problem_id=""

    require_db_env

    problem_id="$(
        PGPASSWORD="${DB_PASSWORD}" psql \
            -X \
            -h "${DB_HOST}" \
            -p "${DB_PORT}" \
            -U "${DB_USER}" \
            -d "${DB_NAME}" \
            -v problem_title="${problem_title}" \
            -v ON_ERROR_STOP=1 \
            -qAt <<'SQL' | sed -n '1p'
WITH created_problem AS (
    INSERT INTO problems(version, title)
    VALUES(1, :'problem_title')
    RETURNING problem_id
)
INSERT INTO problem_statistics(problem_id)
SELECT problem_id
FROM created_problem
RETURNING problem_id;
SQL
    )"

    if [[ -z "${problem_id}" ]]; then
        append_log_line "${test_log_temp_file}" "problem create failed"
        publish_fixture_failure_logs
        echo "${failure_context} problem create failed" >&2
        exit 1
    fi

    append_log_line \
        "${test_log_temp_file}" \
        "problem created: problem_id=${problem_id}, title=${problem_title}"
    printf '%s\n' "${problem_id}"
}
