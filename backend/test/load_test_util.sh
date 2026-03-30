#!/usr/bin/env bash

load_test_util_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
load_test_project_root="$(cd "${load_test_util_dir}/.." && pwd)"

# shellcheck disable=SC1091
source "${load_test_util_dir}/util.sh"
# shellcheck disable=SC1091
source "${load_test_util_dir}/flow_test_util.sh"
# shellcheck disable=SC1091
source "${load_test_util_dir}/database_util.sh"
# shellcheck disable=SC1091
source "${load_test_util_dir}/http_server_util.sh"
# shellcheck disable=SC1091
source "${load_test_util_dir}/fixture_util.sh"

load_test_original_db_name="${DB_NAME-}"
load_test_original_db_name_was_set="0"
if [[ "${DB_NAME+x}" == "x" ]]; then
    load_test_original_db_name_was_set="1"
fi

load_backend_env(){
    local env_file="${load_test_project_root}/.env"

    if [[ -f "${env_file}" ]]; then
        set -a
        # shellcheck disable=SC1090
        source "${env_file}"
        set +a
    fi
}

require_positive_integer(){
    local value_name="$1"
    local value_text="$2"

    if [[ -z "${value_name}" || -z "${value_text}" ]]; then
        echo "missing positive integer validation argument" >&2
        return 1
    fi

    if [[ ! "${value_text}" =~ ^[1-9][0-9]*$ ]]; then
        echo "invalid ${value_name}: ${value_text}" >&2
        return 1
    fi
}

make_database_url(){
    local db_name="$1"

    if [[ -z "${db_name}" ]]; then
        echo "missing db_name" >&2
        return 1
    fi

    python3 - "${DB_USER}" "${DB_PASSWORD}" "${DB_HOST}" "${DB_PORT}" "${db_name}" <<'PY'
import sys
import urllib.parse

db_user, db_password, db_host, db_port, db_name = sys.argv[1:]

print(
    "postgresql://"
    f"{urllib.parse.quote(db_user, safe='')}:"
    f"{urllib.parse.quote(db_password, safe='')}"
    f"@{db_host}:{db_port}/{urllib.parse.quote(db_name, safe='')}"
)
PY
}

setup_isolated_test_database(){
    local name_prefix="$1"

    if [[ -z "${name_prefix}" ]]; then
        echo "missing name_prefix" >&2
        return 1
    fi

    require_db_env
    require_db_admin_env

    test_db_name="${name_prefix}_$$_$(date +%s)"
    test_database_url="$(make_database_url "${test_db_name}")"

    create_test_database
    apply_test_database_migrations

    DB_NAME="${test_db_name}"
    export DB_NAME

    if [[ -n "${test_log_temp_file:-}" ]]; then
        append_log_line \
            "${test_log_temp_file}" \
            "isolated test database ready: ${test_db_name}"
    fi
}

restore_original_db_name(){
    if [[ "${load_test_original_db_name_was_set}" == "1" ]]; then
        DB_NAME="${load_test_original_db_name}"
        export DB_NAME
        return 0
    fi

    unset DB_NAME
}

teardown_isolated_test_database(){
    drop_test_database
    restore_original_db_name
}

now_ms(){
    date +%s%3N
}

record_metric(){
    local metrics_file_path="$1"
    local scenario_name="$2"
    local is_success="$3"
    local status_label="$4"
    local duration_ms="$5"

    if [[ -z "${metrics_file_path}" || -z "${scenario_name}" || -z "${duration_ms}" ]]; then
        echo "missing metric argument" >&2
        return 1
    fi

    status_label="${status_label//$'\t'/ }"
    status_label="${status_label//$'\n'/ }"

    printf '%s\t%s\t%s\t%s\n' \
        "${scenario_name}" \
        "${is_success}" \
        "${status_label}" \
        "${duration_ms}" \
        >> "${metrics_file_path}"
}

count_metric_failures(){
    local metrics_file_path="$1"

    if [[ -z "${metrics_file_path}" ]]; then
        echo "missing metrics_file_path" >&2
        return 1
    fi

    python3 - "${metrics_file_path}" <<'PY'
import sys

failure_count = 0

with open(sys.argv[1], encoding="utf-8") as metrics_file:
    for line in metrics_file:
        line = line.rstrip("\n")
        if not line:
            continue

        parts = line.split("\t")
        if len(parts) != 4:
            failure_count += 1
            continue

        _, is_success, _, _ = parts
        if is_success != "1":
            failure_count += 1

print(failure_count)
PY
}

print_metric_summary(){
    local metrics_file_path="$1"
    local summary_label="$2"
    local total_elapsed_ms="$3"

    if [[ -z "${metrics_file_path}" || -z "${summary_label}" || -z "${total_elapsed_ms}" ]]; then
        echo "missing metric summary argument" >&2
        return 1
    fi

    python3 - "${metrics_file_path}" "${summary_label}" "${total_elapsed_ms}" <<'PY'
import collections
import sys


def percentile(sorted_values, ratio):
    if not sorted_values:
        return 0

    if len(sorted_values) == 1:
        return sorted_values[0]

    index = int(round((len(sorted_values) - 1) * ratio))
    return sorted_values[index]


def summarize(entries):
    durations = sorted(duration for _, duration in entries)
    total = len(entries)
    success = sum(1 for is_success, _ in entries if is_success)
    failure = total - success
    average = sum(durations) / total if total else 0.0

    return {
        "total": total,
        "success": success,
        "failure": failure,
        "average": average,
        "min": durations[0] if durations else 0,
        "max": durations[-1] if durations else 0,
        "p50": percentile(durations, 0.50),
        "p95": percentile(durations, 0.95),
        "p99": percentile(durations, 0.99),
    }


metrics_file_path = sys.argv[1]
summary_label = sys.argv[2]
total_elapsed_ms = int(sys.argv[3])

grouped_entries = collections.defaultdict(list)
all_entries = []

with open(metrics_file_path, encoding="utf-8") as metrics_file:
    for raw_line in metrics_file:
        raw_line = raw_line.rstrip("\n")
        if not raw_line:
            continue

        parts = raw_line.split("\t")
        if len(parts) != 4:
            continue

        scenario_name, is_success_text, _, duration_ms_text = parts
        duration_ms = int(duration_ms_text)
        is_success = is_success_text == "1"

        grouped_entries[scenario_name].append((is_success, duration_ms))
        all_entries.append((is_success, duration_ms))

overall_summary = summarize(all_entries)
throughput_rps = 0.0
if total_elapsed_ms > 0:
    throughput_rps = overall_summary["total"] / (total_elapsed_ms / 1000.0)

print(f"{summary_label} total_elapsed_ms={total_elapsed_ms}")
print(
    "overall "
    f"total={overall_summary['total']} "
    f"ok={overall_summary['success']} "
    f"fail={overall_summary['failure']} "
    f"throughput_rps={throughput_rps:.2f} "
    f"avg_ms={overall_summary['average']:.2f} "
    f"p50_ms={overall_summary['p50']} "
    f"p95_ms={overall_summary['p95']} "
    f"p99_ms={overall_summary['p99']} "
    f"min_ms={overall_summary['min']} "
    f"max_ms={overall_summary['max']}"
)

for scenario_name in sorted(grouped_entries):
    scenario_summary = summarize(grouped_entries[scenario_name])
    print(
        f"{scenario_name} "
        f"total={scenario_summary['total']} "
        f"ok={scenario_summary['success']} "
        f"fail={scenario_summary['failure']} "
        f"avg_ms={scenario_summary['average']:.2f} "
        f"p50_ms={scenario_summary['p50']} "
        f"p95_ms={scenario_summary['p95']} "
        f"p99_ms={scenario_summary['p99']} "
        f"min_ms={scenario_summary['min']} "
        f"max_ms={scenario_summary['max']}"
    )
PY
}

cleanup_judge_server(){
    if [[ -n "${judge_server_pid:-}" ]]; then
        kill "${judge_server_pid}" >/dev/null 2>&1 || true
        wait "${judge_server_pid}" >/dev/null 2>&1 || true
    fi
}

publish_judge_server_failure_log(){
    if [[ -z "${judge_server_log_temp_file:-}" || -z "${judge_server_log_name:-}" ]]; then
        return 0
    fi

    if [[ -z "${judge_server_log_path:-}" ]] && [[ -n "${judge_server_pid:-}" || -s "${judge_server_log_temp_file}" ]]; then
        judge_server_log_path="$(
            publish_log_file "${judge_server_log_temp_file}" "${judge_server_log_name}"
        )"
        print_log_file_created "${judge_server_log_path}"
    fi
}

publish_extended_failure_logs(){
    if [[ "$(type -t publish_failure_logs || true)" == "function" ]]; then
        publish_failure_logs
    fi

    publish_judge_server_failure_log
}

ensure_dedicated_judge_server(){
    if [[ -z "${judge_server_bin:-}" ]]; then
        echo "missing judge_server_bin" >&2
        return 1
    fi
    if [[ -z "${judge_server_log_temp_file:-}" || -z "${test_log_temp_file:-}" ]]; then
        echo "missing judge_server log environment" >&2
        return 1
    fi

    if [[ ! -x "${judge_server_bin}" ]]; then
        echo "judge_server binary not found or not executable: ${judge_server_bin}" >&2
        append_log_line "${test_log_temp_file}" "judge_server binary not found: ${judge_server_bin}"
        publish_extended_failure_logs
        echo "hint: run '${load_test_project_root}/scripts/build_backend.sh'" >&2
        return 1
    fi

    append_log_line "${test_log_temp_file}" "starting dedicated judge_server"
    "${judge_server_bin}" >"${judge_server_log_temp_file}" 2>&1 &
    judge_server_pid="$!"

    local attempt=0
    while (( attempt < 30 )); do
        if ! kill -0 "${judge_server_pid}" >/dev/null 2>&1; then
            echo "failed to start dedicated judge_server" >&2
            append_log_line "${test_log_temp_file}" "failed to start dedicated judge_server"
            publish_extended_failure_logs
            echo "judge server log:" >&2
            cat "${judge_server_log_temp_file}" >&2
            return 1
        fi

        if (( attempt >= 4 )); then
            return 0
        fi

        sleep 0.1
        attempt=$((attempt + 1))
    done

    return 0
}

make_limits_request_body(){
    python3 - "$1" "$2" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "memory_limit_mb": int(sys.argv[1]),
            "time_limit_ms": int(sys.argv[2]),
        }
    )
)
PY
}

make_statement_request_body(){
    python3 - "$1" "$2" "$3" "$4" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "description": sys.argv[1],
            "input_format": sys.argv[2],
            "output_format": sys.argv[3],
            "note": sys.argv[4],
        }
    )
)
PY
}

make_sample_request_body(){
    python3 - "$1" "$2" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "sample_input": sys.argv[1],
            "sample_output": sys.argv[2],
        }
    )
)
PY
}

make_testcase_request_body(){
    python3 - "$1" "$2" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "testcase_input": sys.argv[1],
            "testcase_output": sys.argv[2],
        }
    )
)
PY
}

make_submission_request_body(){
    python3 - "$1" "$2" <<'PY'
import json
import sys

print(
    json.dumps(
        {
            "language": sys.argv[1],
            "source_code": sys.argv[2],
        }
    )
)
PY
}

read_submission_id_from_response(){
    local response_file_path="$1"

    python3 - "${response_file_path}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as response_file:
    response = json.load(response_file)

submission_id = response.get("submission_id")
if not isinstance(submission_id, int) or submission_id <= 0:
    raise SystemExit("invalid submission_id in response")

print(submission_id)
PY
}
