#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "${script_dir}/.." && pwd)"

# shellcheck disable=SC1091
source "${script_dir}/util.sh"
# shellcheck disable=SC1091
source "${script_dir}/flow_test_util.sh"
# shellcheck disable=SC1091
source "${script_dir}/database_util.sh"
# shellcheck disable=SC1091
source "${script_dir}/http_server_util.sh"
# shellcheck disable=SC1091
source "${script_dir}/fixture_util.sh"

if [[ -f "${project_root}/.env" ]]; then
    set -a
    # shellcheck disable=SC1091
    source "${project_root}/.env"
    set +a
fi

http_port="${SYSTEM_FLOW_TEST_HTTP_PORT:-18089}"
base_url="${SYSTEM_FLOW_TEST_BASE_URL:-http://127.0.0.1:${http_port}}"
http_server_bin="${SYSTEM_FLOW_TEST_HTTP_SERVER_BIN:-${project_root}/http_server}"
regular_user_login_id="${SYSTEM_FLOW_TEST_USER_LOGIN_ID:-$(make_test_login_id syu)}"
admin_user_login_id="${SYSTEM_FLOW_TEST_ADMIN_LOGIN_ID:-$(make_test_login_id sya)}"
raw_password="${SYSTEM_FLOW_TEST_PASSWORD:-password123}"
test_db_name="system_flow_test_$$_$(date +%s)"
test_database_created="0"
test_log_path=""
server_log_path=""
server_pid=""
test_log_name="test_system_flow.log"
server_log_name="test_system_flow_server.log"

init_flow_test
register_temp_file test_log_temp_file
register_temp_file server_log_temp_file
register_temp_file health_response_file
register_temp_file health_response_header_file
register_temp_file supported_languages_response_file
register_temp_file supported_languages_response_header_file
register_temp_file regular_sign_up_response_file
register_temp_file admin_sign_up_response_file
register_temp_file status_unauthorized_response_file
register_temp_file status_response_file

seed_queued_submission(){
    local user_id="$1"
    local problem_id="$2"
    local failure_context="${3:-system flow}"
    local submission_id=""

    submission_id="$(
        PGPASSWORD="${DB_PASSWORD}" psql \
            -X \
            -h "${DB_HOST}" \
            -p "${DB_PORT}" \
            -U "${DB_USER}" \
            -d "${DB_NAME}" \
            -v user_id="${user_id}" \
            -v problem_id="${problem_id}" \
            -v ON_ERROR_STOP=1 \
            -qAt <<'SQL' | sed -n '1p'
WITH created_submission AS (
    INSERT INTO submissions(user_id, problem_id, problem_version, language, source_code)
    VALUES(:'user_id', :'problem_id', 1, 'cpp', 'int main(){return 0;}')
    RETURNING submission_id
)
INSERT INTO submission_queue(submission_id, priority)
SELECT submission_id, 0
FROM created_submission
RETURNING submission_id;
SQL
    )"

    if [[ -z "${submission_id}" ]]; then
        append_log_line "${test_log_temp_file}" "failed to seed queued submission"
        publish_failure_logs
        echo "${failure_context} queued submission seed failed" >&2
        exit 1
    fi

    append_log_line "${test_log_temp_file}" "seeded queued submission: submission_id=${submission_id}, problem_id=${problem_id}, user_id=${user_id}"
    printf '%s\n' "${submission_id}"
}

trap 'finish_flow_test cleanup_http_server drop_test_database' EXIT

require_command curl
require_command psql
require_command python3

export DB_ADMIN_USER="${DB_ADMIN_USER:-${DB_USER:-postgres}}"
export DB_ADMIN_PASSWORD="${DB_ADMIN_PASSWORD:-${DB_PASSWORD:-postgres}}"
test_database_url="postgresql://${DB_USER}:${DB_PASSWORD}@${DB_HOST}:${DB_PORT}/${test_db_name}"
create_test_database
export DB_NAME="${test_db_name}"

append_log_line "${test_log_temp_file}" "base_url=${base_url}"
append_log_line "${test_log_temp_file}" "test_db_name=${test_db_name}"
append_log_line "${test_log_temp_file}" "regular_user_login_id=${regular_user_login_id}"
append_log_line "${test_log_temp_file}" "admin_user_login_id=${admin_user_login_id}"

apply_test_database_migrations
ensure_dedicated_http_server

read -r regular_user_id regular_user_token < <(
    sign_up_user \
        "${regular_user_login_id}" \
        "${raw_password}" \
        "${regular_sign_up_response_file}" \
        "system flow regular user"
)
print_success_log "system flow regular user sign-up success"

read -r admin_user_id admin_user_token < <(
    sign_up_user \
        "${admin_user_login_id}" \
        "${raw_password}" \
        "${admin_sign_up_response_file}" \
        "system flow admin user"
)
print_success_log "system flow admin user sign-up success"

promote_admin_user "${admin_user_id}" "system flow" >/dev/null
print_success_log "system flow admin promotion success"

problem_id="$(
    create_problem_in_db "system flow" "system flow queued problem"
)"
queued_submission_id="$(
    seed_queued_submission "${regular_user_id}" "${problem_id}" "system flow"
)"
print_success_log "system flow queued submission seed success"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/system/health" \
    "${health_response_file}" \
    "200" \
    "system health" \
    "" \
    "" \
    "application/json" \
    "${health_response_header_file}"
assert_json_message \
    "${health_response_file}" \
    "ok" \
    "system health"
health_request_id="$(
    assert_http_header_nonempty \
        "${health_response_header_file}" \
        "X-Request-Id" \
        "system health request id"
)"

print_success_log "system health validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/system/supported-languages" \
    "${supported_languages_response_file}" \
    "200" \
    "supported languages" \
    "" \
    "" \
    "application/json" \
    "${supported_languages_response_header_file}"
assert_json_field_equals \
    "${supported_languages_response_file}" \
    "language_count" \
    "3" \
    "supported languages" \
    "int"
supported_languages_request_id="$(
    assert_http_header_nonempty \
        "${supported_languages_response_header_file}" \
        "X-Request-Id" \
        "supported languages request id"
)"

if [[ "${health_request_id}" == "${supported_languages_request_id}" ]]; then
    append_log_line "${test_log_temp_file}" "request id header validation failed: duplicate request ids"
    publish_failure_logs
    echo "expected unique X-Request-Id values per request" >&2
    exit 1
fi

if ! python3 - "${supported_languages_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

expected_languages = [
    {
        "language": "cpp",
        "source_extension": ".cpp",
    },
    {
        "language": "python",
        "source_extension": ".py",
    },
    {
        "language": "java",
        "source_extension": ".java",
    },
]

if response.get("languages") != expected_languages:
    raise SystemExit("supported language list mismatch")
PY
then
    append_log_line "${test_log_temp_file}" "supported language response validation failed"
    publish_failure_logs
    exit 1
fi

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/system/status" \
    "${status_unauthorized_response_file}" \
    "401" \
    "system status unauthorized" \
    "${regular_user_token}"
assert_json_error_code \
    "${status_unauthorized_response_file}" \
    "admin_bearer_token_required" \
    "system status unauthorized"
print_success_log "system status admin guard validated"

send_http_request_and_assert_status \
    "GET" \
    "${base_url}/api/system/status" \
    "${status_response_file}" \
    "200" \
    "system status" \
    "${admin_user_token}"

if ! python3 - "${status_response_file}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

http_value = response.get("http")
judge_value = response.get("judge")
if not isinstance(http_value, dict) or not isinstance(judge_value, dict):
    raise SystemExit("missing system status sections")

db_pool = http_value.get("db_pool")
handler_queue = http_value.get("handler_queue")
if not isinstance(db_pool, dict) or not isinstance(handler_queue, dict):
    raise SystemExit("missing http runtime sections")

size = db_pool.get("size")
available_count = db_pool.get("available_count")
if not isinstance(size, int) or size <= 0:
    raise SystemExit("invalid db_pool.size")
if not isinstance(available_count, int) or available_count < 0 or available_count > size:
    raise SystemExit("invalid db_pool.available_count")

backlog_count = handler_queue.get("backlog_count")
limit = handler_queue.get("limit")
if backlog_count != 0:
    raise SystemExit(f"unexpected handler backlog count: {backlog_count!r}")
if limit is not None and (not isinstance(limit, int) or limit <= 0):
    raise SystemExit(f"invalid handler queue limit: {limit!r}")

if judge_value.get("queue_depth") != 1:
    raise SystemExit(f"unexpected queue depth: {judge_value.get('queue_depth')!r}")
if judge_value.get("live_instance_count") != 0:
    raise SystemExit("expected no live judge instances")
if judge_value.get("configured_worker_count") != 0:
    raise SystemExit("expected zero configured judge workers without judge server")
if judge_value.get("active_worker_count") != 0:
    raise SystemExit("expected zero active judge workers without judge server")

snapshot_cache = judge_value.get("snapshot_cache")
if snapshot_cache != {"hit_count": 0, "miss_count": 0}:
    raise SystemExit(f"unexpected snapshot cache aggregate: {snapshot_cache!r}")

self_check = judge_value.get("last_sandbox_self_check")
if not isinstance(self_check, dict):
    raise SystemExit("missing last_sandbox_self_check object")
if self_check.get("status") != "unknown":
    raise SystemExit(f"expected unknown self check status, got {self_check.get('status')!r}")
if self_check.get("checked_at") is not None:
    raise SystemExit("expected null self check checked_at without judge server")
if self_check.get("message") != "judge has not reported self-check yet":
    raise SystemExit(f"unexpected self check message: {self_check.get('message')!r}")
PY
then
    append_log_line "${test_log_temp_file}" "system status response validation failed"
    publish_failure_logs
    exit 1
fi

append_log_line \
    "${test_log_temp_file}" \
    "system status validated: queued_submission_id=${queued_submission_id}, problem_id=${problem_id}"
print_success_log "system status validated"

append_log_line "${test_log_temp_file}" "system flow test passed"
print_success_log "system flow test passed"
