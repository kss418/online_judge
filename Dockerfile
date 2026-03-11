FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        bash \
        bison \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        flex \
        git \
        ninja-build \
        perl \
        pkg-config \
        python3 \
        unzip \
        zip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . .

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/workspace/vcpkg/scripts/buildsystems/vcpkg.cmake \
    && cmake --build build -j --target http_server judge_server

FROM ubuntu:24.04 AS runtime_base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

FROM runtime_base AS http_runtime
COPY --from=build /workspace/http_server /app/http_server
ENV HTTP_PORT=8080
EXPOSE 8080
CMD ["./http_server"]

FROM runtime_base AS judge_runtime
ARG JUDGE_CPP_COMPILER_PATH=""
ARG JUDGE_PYTHON_PATH=""
ARG JUDGE_JAVA_RUNTIME_PATH=""

RUN set -eux; \
    packages=""; \
    if [ -n "${JUDGE_CPP_COMPILER_PATH}" ]; then \
        cpp_compiler_name="$(basename "${JUDGE_CPP_COMPILER_PATH}")"; \
        if [ "${cpp_compiler_name}" != "g++" ]; then \
            echo "unsupported JUDGE_CPP_COMPILER_PATH: ${JUDGE_CPP_COMPILER_PATH}" >&2; \
            exit 1; \
        fi; \
        packages="${packages} g++"; \
    fi; \
    if [ -n "${JUDGE_PYTHON_PATH}" ]; then \
        python_name="$(basename "${JUDGE_PYTHON_PATH}")"; \
        if [ "${python_name}" != "python3" ]; then \
            echo "unsupported JUDGE_PYTHON_PATH: ${JUDGE_PYTHON_PATH}" >&2; \
            exit 1; \
        fi; \
        packages="${packages} python3"; \
    fi; \
    if [ -n "${JUDGE_JAVA_RUNTIME_PATH}" ]; then \
        java_name="$(basename "${JUDGE_JAVA_RUNTIME_PATH}")"; \
        if [ "${java_name}" != "java" ]; then \
            echo "unsupported JUDGE_JAVA_RUNTIME_PATH: ${JUDGE_JAVA_RUNTIME_PATH}" >&2; \
            exit 1; \
        fi; \
        packages="${packages} openjdk-21-jdk-headless"; \
    fi; \
    if [ -n "${packages}" ]; then \
        apt-get update; \
        apt-get install -y --no-install-recommends ${packages}; \
        rm -rf /var/lib/apt/lists/*; \
    fi; \
    if [ -n "${JUDGE_CPP_COMPILER_PATH}" ] && [ "${JUDGE_CPP_COMPILER_PATH}" != "/usr/bin/g++" ]; then \
        mkdir -p "$(dirname "${JUDGE_CPP_COMPILER_PATH}")"; \
        ln -sf /usr/bin/g++ "${JUDGE_CPP_COMPILER_PATH}"; \
    fi; \
    if [ -n "${JUDGE_PYTHON_PATH}" ] && [ "${JUDGE_PYTHON_PATH}" != "/usr/bin/python3" ]; then \
        mkdir -p "$(dirname "${JUDGE_PYTHON_PATH}")"; \
        ln -sf /usr/bin/python3 "${JUDGE_PYTHON_PATH}"; \
    fi; \
    if [ -n "${JUDGE_JAVA_RUNTIME_PATH}" ] && [ "${JUDGE_JAVA_RUNTIME_PATH}" != "/usr/bin/java" ]; then \
        mkdir -p "$(dirname "${JUDGE_JAVA_RUNTIME_PATH}")"; \
        ln -sf /usr/bin/java "${JUDGE_JAVA_RUNTIME_PATH}"; \
    fi

COPY --from=build /workspace/judge_server /app/judge_server
CMD ["./judge_server"]
