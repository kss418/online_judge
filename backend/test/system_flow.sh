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

apply_test_database_migrations
ensure_dedicated_http_server

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

append_log_line "${test_log_temp_file}" "system flow test passed"
print_success_log "system flow test passed"
