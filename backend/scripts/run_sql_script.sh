#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
backend_root="$(cd "${script_dir}/.." && pwd)"
repo_root="$(cd "${backend_root}/.." && pwd)"

if (($# == 0)); then
    echo "error: sql basename is required" >&2
    echo "usage: $(basename "$0") <sql_basename>" >&2
    exit 1
fi

if (($# > 1)); then
    echo "error: expected exactly 1 argument, got ${#}" >&2
    echo "usage: $(basename "$0") <sql_basename>" >&2
    exit 1
fi

sql_name="$1"
sql_file="${script_dir}/sql/${sql_name}.sql"

if [[ ! -f "${sql_file}" ]]; then
    echo "error: sql file not found: ${sql_file}" >&2
    exit 1
fi

# shellcheck disable=SC1091
source "${repo_root}/scripts/lib/postgres.sh"

source_project_env "${backend_root}"
require_psql

echo "apply ${sql_name}"
psql_run -f "${sql_file}"

echo "migration completed"
