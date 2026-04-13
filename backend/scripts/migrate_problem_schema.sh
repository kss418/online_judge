#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
backend_root="$(cd "${script_dir}/.." && pwd)"
repo_root="$(cd "${backend_root}/.." && pwd)"
sql_file="${script_dir}/sql/migrate_problem_schema.sql"

# shellcheck disable=SC1091
source "${repo_root}/scripts/lib/postgres.sh"

source_project_env "${backend_root}"
require_psql

echo "apply problem_schema"
psql_run -f "${sql_file}"

echo "migration completed"
