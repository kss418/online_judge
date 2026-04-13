#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
backend_root="$(cd "${script_dir}/.." && pwd)"
repo_root="$(cd "${backend_root}/.." && pwd)"
count_sql_file="${script_dir}/sql/validate_user_problem_summary_count.sql"
details_sql_file="${script_dir}/sql/validate_user_problem_summary_details.sql"

# shellcheck disable=SC1091
source "${repo_root}/scripts/lib/postgres.sh"

source_project_env "${backend_root}"
require_psql

diff_count="$(
    psql_run -qAt -f "${count_sql_file}"
)"

if [[ "${diff_count}" != "0" ]]; then
    echo "user_problem_attempt_summary validation failed" >&2
    psql_run -f "${details_sql_file}"
    exit 1
fi

echo "user_problem_attempt_summary validation passed"
