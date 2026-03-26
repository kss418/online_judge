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

send_http_request(){
    local request_method="$1"
    local request_url="$2"
    local response_file_path="$3"
    local auth_token="${4:-}"
    local request_body="${5:-}"
    local content_type="${6:-application/json}"
    local curl_args=()

    require_fixture_test_env

    if [[ -z "${request_method}" || -z "${request_url}" || -z "${response_file_path}" ]]; then
        echo "missing required http request argument" >&2
        exit 1
    fi

    curl_args=(
        curl
        --silent
        --show-error
        --output "${response_file_path}"
        --write-out "%{http_code}"
        --request "${request_method}"
    )

    if [[ -n "${auth_token}" ]]; then
        curl_args+=(-H "Authorization: Bearer ${auth_token}")
    fi

    if [[ -n "${request_body}" ]]; then
        curl_args+=(-H "Content-Type: ${content_type}")
        curl_args+=(-d "${request_body}")
    fi

    curl_args+=("${request_url}")

    "${curl_args[@]}"
}

send_http_request_and_assert_status(){
    local request_method="$1"
    local request_url="$2"
    local response_file_path="$3"
    local expected_status_code="$4"
    local failure_context="$5"
    local auth_token="${6:-}"
    local request_body="${7:-}"
    local content_type="${8:-application/json}"
    local actual_status_code=""

    actual_status_code="$(
        send_http_request \
            "${request_method}" \
            "${request_url}" \
            "${response_file_path}" \
            "${auth_token}" \
            "${request_body}" \
            "${content_type}"
    )"

    assert_status_code \
        "${actual_status_code}" \
        "${expected_status_code}" \
        "${response_file_path}" \
        "${failure_context}"
}

assert_status_code(){
    local actual_status_code="$1"
    local expected_status_code="$2"
    local response_file_path="$3"
    local failure_context="$4"

    if [[ -z "${actual_status_code}" || -z "${expected_status_code}" || -z "${response_file_path}" || -z "${failure_context}" ]]; then
        echo "missing required status assertion argument" >&2
        exit 1
    fi

    if [[ "${actual_status_code}" != "${expected_status_code}" ]]; then
        append_log_line "${test_log_temp_file}" "${failure_context} failed: status=${actual_status_code}"
        publish_fixture_failure_logs
        echo "${failure_context}: expected status ${expected_status_code}, got ${actual_status_code}" >&2
        echo "response body:" >&2
        cat "${response_file_path}" >&2
        exit 1
    fi

    append_log_line "${test_log_temp_file}" "${failure_context} passed: status=${actual_status_code}"
}

read_json_field(){
    local response_file_path="$1"
    local field_path="$2"
    local expected_type="${3:-string}"

    if [[ -z "${response_file_path}" || -z "${field_path}" ]]; then
        echo "missing required read json field argument" >&2
        exit 1
    fi

    python3 - "${response_file_path}" "${field_path}" "${expected_type}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
field_path = sys.argv[2]
expected_type = sys.argv[3]

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

current_value = response
for field_name in field_path.split("."):
    if not isinstance(current_value, dict) or field_name not in current_value:
        raise SystemExit(f"missing json field: {field_path}")
    current_value = current_value[field_name]

if expected_type == "string":
    if not isinstance(current_value, str) or not current_value:
        raise SystemExit(f"expected non-empty string field: {field_path}")
elif expected_type == "int":
    if not isinstance(current_value, int):
        raise SystemExit(f"expected int field: {field_path}")
elif expected_type == "bool":
    if not isinstance(current_value, bool):
        raise SystemExit(f"expected bool field: {field_path}")
else:
    raise SystemExit(f"unsupported expected_type: {expected_type}")

print(current_value)
PY
}

assert_json_field_equals(){
    local response_file_path="$1"
    local field_path="$2"
    local expected_value="$3"
    local failure_context="$4"
    local expected_type="${5:-string}"

    if [[ -z "${response_file_path}" || -z "${field_path}" || -z "${failure_context}" ]]; then
        echo "missing required json field assertion argument" >&2
        exit 1
    fi

    if ! python3 - "${response_file_path}" "${field_path}" "${expected_value}" "${expected_type}" <<'PY'
import json
import sys

response_file_path = sys.argv[1]
field_path = sys.argv[2]
expected_value_text = sys.argv[3]
expected_type = sys.argv[4]

with open(response_file_path, encoding="utf-8") as response_file:
    response = json.load(response_file)

current_value = response
for field_name in field_path.split("."):
    if not isinstance(current_value, dict) or field_name not in current_value:
        raise SystemExit(f"missing json field: {field_path}")
    current_value = current_value[field_name]

if expected_type == "string":
    expected_value = expected_value_text
elif expected_type == "int":
    expected_value = int(expected_value_text)
elif expected_type == "null":
    expected_value = None
elif expected_type == "bool":
    if expected_value_text == "true":
        expected_value = True
    elif expected_value_text == "false":
        expected_value = False
    else:
        raise SystemExit(f"invalid bool expected value: {expected_value_text}")
else:
    raise SystemExit(f"unsupported expected_type: {expected_type}")

if current_value != expected_value:
    raise SystemExit(
        f"json field mismatch: {field_path}, expected={expected_value!r}, actual={current_value!r}"
    )
PY
    then
        append_log_line "${test_log_temp_file}" "${failure_context} json field validation failed: field=${field_path}"
        publish_fixture_failure_logs
        echo "${failure_context} json field validation failed: field=${field_path}" >&2
        echo "response body:" >&2
        cat "${response_file_path}" >&2
        exit 1
    fi
}

assert_json_error_code(){
    local response_file_path="$1"
    local expected_error_code="$2"
    local failure_context="$3"

    assert_json_field_equals \
        "${response_file_path}" \
        "error.code" \
        "${expected_error_code}" \
        "${failure_context}" \
        "string"
}

assert_json_message(){
    local response_file_path="$1"
    local expected_message="$2"
    local failure_context="$3"

    assert_json_field_equals \
        "${response_file_path}" \
        "message" \
        "${expected_message}" \
        "${failure_context}" \
        "string"
}

assert_json_error_message(){
    local response_file_path="$1"
    local expected_message="$2"
    local failure_context="$3"

    assert_json_field_equals \
        "${response_file_path}" \
        "error.message" \
        "${expected_message}" \
        "${failure_context}" \
        "string"
}

assert_json_error_field(){
    local response_file_path="$1"
    local expected_field="$2"
    local failure_context="$3"

    assert_json_field_equals \
        "${response_file_path}" \
        "error.field" \
        "${expected_field}" \
        "${failure_context}" \
        "string"
}

make_sign_up_request_body(){
    local user_login_id="$1"
    local raw_password="$2"
    local user_name="${3:-${user_login_id}}"

    python3 - "${user_login_id}" "${raw_password}" "${user_name}" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "user_login_id": sys.argv[1],
            "raw_password": sys.argv[2],
            "user_name": sys.argv[3],
        }
    )
)
PY
}

make_login_request_body(){
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
    local user_name="${5:-${user_login_id}}"
    local sign_up_request_body=""
    local sign_up_status_code=""

    require_fixture_test_env

    sign_up_request_body="$(
        make_sign_up_request_body "${user_login_id}" "${raw_password}" "${user_name}"
    )"
    sign_up_status_code="$(
        send_http_request \
            "POST" \
            "${base_url}/api/auth/sign-up" \
            "${response_file_path}" \
            "" \
            "${sign_up_request_body}"
    )"

    assert_status_code \
        "${sign_up_status_code}" \
        "201" \
        "${response_file_path}" \
        "${failure_context} sign-up"

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
SET permission_level = 1, updated_at = NOW()
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
        send_http_request \
            "POST" \
            "${base_url}/api/problem" \
            "${response_file_path}" \
            "${auth_token}" \
            "${create_problem_request_body}"
    )"

    assert_status_code \
        "${create_problem_status_code}" \
        "201" \
        "${response_file_path}" \
        "${failure_context} create problem"

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
