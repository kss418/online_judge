#!/usr/bin/env bash

resolve_cpp_compiler_path(){
    local compiler_path=""

    compiler_path="$(resolve_command_path c++)"
    if [[ -n "${compiler_path}" ]]; then
        printf '%s\n' "${compiler_path}"
        return 0
    fi

    resolve_command_path g++
}

ensure_judge_runtime_env(){
    local missing_requirements=()

    export JUDGE_SOURCE_ROOT="${JUDGE_SOURCE_ROOT:-/tmp/online_judge/submissions}"
    export TESTCASE_PATH="${TESTCASE_PATH:-/tmp/online_judge/testcases}"
    export JUDGE_NSJAIL_PATH="${JUDGE_NSJAIL_PATH:-$(resolve_command_path nsjail)}"
    export JUDGE_CPP_COMPILER_PATH="${JUDGE_CPP_COMPILER_PATH:-$(resolve_cpp_compiler_path)}"
    export JUDGE_PYTHON_PATH="${JUDGE_PYTHON_PATH:-$(resolve_command_path python3)}"
    export JUDGE_JAVA_COMPILER_PATH="${JUDGE_JAVA_COMPILER_PATH:-$(resolve_command_path javac)}"
    export JUDGE_JAVA_RUNTIME_PATH="${JUDGE_JAVA_RUNTIME_PATH:-$(resolve_command_path java)}"

    [[ -n "${JUDGE_NSJAIL_PATH}" && -x "${JUDGE_NSJAIL_PATH}" ]] || missing_requirements+=("nsjail or JUDGE_NSJAIL_PATH")
    [[ -n "${JUDGE_CPP_COMPILER_PATH}" && -x "${JUDGE_CPP_COMPILER_PATH}" ]] || missing_requirements+=("C++ compiler (c++/g++) or JUDGE_CPP_COMPILER_PATH")
    [[ -n "${JUDGE_PYTHON_PATH}" && -x "${JUDGE_PYTHON_PATH}" ]] || missing_requirements+=("Python 3 or JUDGE_PYTHON_PATH")
    [[ -n "${JUDGE_JAVA_COMPILER_PATH}" && -x "${JUDGE_JAVA_COMPILER_PATH}" ]] || missing_requirements+=("Java compiler (javac) or JUDGE_JAVA_COMPILER_PATH")
    [[ -n "${JUDGE_JAVA_RUNTIME_PATH}" && -x "${JUDGE_JAVA_RUNTIME_PATH}" ]] || missing_requirements+=("Java runtime (java) or JUDGE_JAVA_RUNTIME_PATH")

    if (( ${#missing_requirements[@]} > 0 )); then
        echo "error: judge runtime prerequisites are missing" >&2
        printf '  - %s\n' "${missing_requirements[@]}" >&2
        echo "hint: install the missing tools or set the corresponding JUDGE_* paths in backend/.env" >&2
        return 1
    fi

    mkdir -p "${JUDGE_SOURCE_ROOT}" "${TESTCASE_PATH}"
}
