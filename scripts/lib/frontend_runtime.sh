#!/usr/bin/env bash

prepend_frontend_bin_dir(){
    local frontend_bin_dir_path="${1:-}"

    if [[ -z "${frontend_bin_dir_path}" ]]; then
        return 0
    fi

    if [[ ! -d "${frontend_bin_dir_path}" ]]; then
        echo "error: FRONTEND_BIN_DIR does not exist: ${frontend_bin_dir_path}" >&2
        return 1
    fi

    export PATH="${frontend_bin_dir_path}:${PATH}"
}

parse_semver(){
    local version_text="${1#v}"

    if [[ ! "${version_text}" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
        return 1
    fi

    printf '%s %s %s\n' \
        "${BASH_REMATCH[1]}" \
        "${BASH_REMATCH[2]}" \
        "${BASH_REMATCH[3]}"
}

node_version_satisfies_frontend(){
    local version_text="$1"
    local major=0
    local minor=0
    local patch=0

    read -r major minor patch < <(parse_semver "${version_text}") || return 1

    if (( major == 20 )); then
        (( minor > 19 )) && return 0
        (( minor == 19 && patch >= 0 )) && return 0
        return 1
    fi

    if (( major == 21 )); then
        return 1
    fi

    if (( major == 22 )); then
        (( minor > 12 )) && return 0
        (( minor == 12 && patch >= 0 )) && return 0
        return 1
    fi

    (( major > 22 ))
}

check_frontend_runtime(){
    local frontend_node_bin="${1:-node}"
    local frontend_npm_bin="${2:-npm}"
    local node_version=""

    require_command "${frontend_node_bin}"
    require_command "${frontend_npm_bin}"

    node_version="$("${frontend_node_bin}" -v)"

    if ! node_version_satisfies_frontend "${node_version}"; then
        echo "error: frontend requires Node.js ^20.19.0 or >=22.12.0, but found ${node_version}" >&2
        echo "node path: $(command -v "${frontend_node_bin}")" >&2
        echo "hint: install Node.js 22.12.0+ or 20.19.0+ before running this script" >&2
        echo "hint: you can also run with FRONTEND_BIN_DIR=/path/to/node/bin ./scripts/dev_tunnel.sh" >&2
        echo "hint: or set FRONTEND_NODE_BIN and FRONTEND_NPM_BIN explicitly" >&2
        return 1
    fi
}

build_allowed_hosts_csv(){
    local frontend_env_local="$1"
    local tunnel_host="$2"

    {
        if [[ -n "${VITE_ALLOWED_HOSTS:-}" ]]; then
            printf '%s\n' "${VITE_ALLOWED_HOSTS}" | tr ',' '\n'
        fi

        if [[ -f "${frontend_env_local}" ]]; then
            sed -n 's/^VITE_ALLOWED_HOSTS=//p' "${frontend_env_local}" | tail -n 1 | tr ',' '\n'
        fi

        if [[ -n "${EXTRA_VITE_ALLOWED_HOSTS:-}" ]]; then
            printf '%s\n' "${EXTRA_VITE_ALLOWED_HOSTS}" | tr ',' '\n'
        fi
    } | awk -v tunnel_host="${tunnel_host}" '
        {
            gsub(/^[[:space:]]+|[[:space:]]+$/, "", $0)
            if($0 == "" || $0 ~ /\.trycloudflare\.com$/){
                next
            }

            if(!seen[$0]++){
                hosts[count++] = $0
            }
        }
        END {
            if(tunnel_host != "" && !seen[tunnel_host]++){
                hosts[count++] = tunnel_host
            }

            for(i = 0; i < count; ++i){
                printf("%s", hosts[i])
                if(i + 1 < count){
                    printf(",")
                }
            }
        }
    '
}
