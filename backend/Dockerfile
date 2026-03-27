FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_DISABLE_METRICS=1

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        bash \
        bison \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        flex \
        git \
        libnl-route-3-dev \
        libprotobuf-dev \
        libtool \
        ninja-build \
        openjdk-21-jdk-headless \
        perl \
        pkg-config \
        protobuf-compiler \
        python3 \
        unzip \
        zip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY vcpkg.json /workspace/vcpkg.json
COPY vcpkg /workspace/vcpkg

RUN /workspace/vcpkg/vcpkg install \
        --triplet=x64-linux \
        --x-manifest-root=/workspace

COPY . .

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/workspace/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_INSTALLED_DIR=/workspace/vcpkg_installed \
    && cmake --build build -j --target http_server judge_server

RUN git clone --depth 1 https://github.com/google/nsjail.git /tmp/nsjail \
    && make -C /tmp/nsjail \
    && install -Dm755 /tmp/nsjail/nsjail /usr/local/bin/nsjail

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

FROM build AS judge_runtime
WORKDIR /app

ARG JUDGE_NSJAIL_PATH=""
ARG JUDGE_CPP_COMPILER_PATH=""
ARG JUDGE_PYTHON_PATH=""
ARG JUDGE_JAVA_COMPILER_PATH=""
ARG JUDGE_JAVA_RUNTIME_PATH=""

RUN set -eux; \
    if [ -n "${JUDGE_NSJAIL_PATH}" ]; then \
        nsjail_name="$(basename "${JUDGE_NSJAIL_PATH}")"; \
        if [ "${nsjail_name}" != "nsjail" ]; then \
            echo "unsupported JUDGE_NSJAIL_PATH: ${JUDGE_NSJAIL_PATH}" >&2; \
            exit 1; \
        fi; \
    fi; \
    if [ -n "${JUDGE_CPP_COMPILER_PATH}" ]; then \
        cpp_compiler_name="$(basename "${JUDGE_CPP_COMPILER_PATH}")"; \
        if [ "${cpp_compiler_name}" != "g++" ]; then \
            echo "unsupported JUDGE_CPP_COMPILER_PATH: ${JUDGE_CPP_COMPILER_PATH}" >&2; \
            exit 1; \
        fi; \
    fi; \
    if [ -n "${JUDGE_PYTHON_PATH}" ]; then \
        python_name="$(basename "${JUDGE_PYTHON_PATH}")"; \
        if [ "${python_name}" != "python3" ]; then \
            echo "unsupported JUDGE_PYTHON_PATH: ${JUDGE_PYTHON_PATH}" >&2; \
            exit 1; \
        fi; \
    fi; \
    if [ -n "${JUDGE_JAVA_COMPILER_PATH}" ]; then \
        javac_name="$(basename "${JUDGE_JAVA_COMPILER_PATH}")"; \
        if [ "${javac_name}" != "javac" ]; then \
            echo "unsupported JUDGE_JAVA_COMPILER_PATH: ${JUDGE_JAVA_COMPILER_PATH}" >&2; \
            exit 1; \
        fi; \
    fi; \
    if [ -n "${JUDGE_JAVA_RUNTIME_PATH}" ]; then \
        java_name="$(basename "${JUDGE_JAVA_RUNTIME_PATH}")"; \
        if [ "${java_name}" != "java" ]; then \
            echo "unsupported JUDGE_JAVA_RUNTIME_PATH: ${JUDGE_JAVA_RUNTIME_PATH}" >&2; \
            exit 1; \
        fi; \
    fi; \
    install -Dm755 /workspace/judge_server /app/judge_server; \
    if [ -n "${JUDGE_CPP_COMPILER_PATH}" ] && [ "${JUDGE_CPP_COMPILER_PATH}" != "/usr/bin/g++" ]; then \
        mkdir -p "$(dirname "${JUDGE_CPP_COMPILER_PATH}")"; \
        ln -sf /usr/bin/g++ "${JUDGE_CPP_COMPILER_PATH}"; \
    fi; \
    if [ -n "${JUDGE_PYTHON_PATH}" ] && [ "${JUDGE_PYTHON_PATH}" != "/usr/bin/python3" ]; then \
        mkdir -p "$(dirname "${JUDGE_PYTHON_PATH}")"; \
        ln -sf /usr/bin/python3 "${JUDGE_PYTHON_PATH}"; \
    fi; \
    if [ -n "${JUDGE_NSJAIL_PATH}" ] && [ "${JUDGE_NSJAIL_PATH}" != "/usr/local/bin/nsjail" ]; then \
        mkdir -p "$(dirname "${JUDGE_NSJAIL_PATH}")"; \
        ln -sf /usr/local/bin/nsjail "${JUDGE_NSJAIL_PATH}"; \
    fi; \
    if [ -n "${JUDGE_JAVA_COMPILER_PATH}" ] && [ "${JUDGE_JAVA_COMPILER_PATH}" != "/usr/bin/javac" ]; then \
        mkdir -p "$(dirname "${JUDGE_JAVA_COMPILER_PATH}")"; \
        ln -sf /usr/bin/javac "${JUDGE_JAVA_COMPILER_PATH}"; \
    fi; \
    if [ -n "${JUDGE_JAVA_RUNTIME_PATH}" ] && [ "${JUDGE_JAVA_RUNTIME_PATH}" != "/usr/bin/java" ]; then \
        mkdir -p "$(dirname "${JUDGE_JAVA_RUNTIME_PATH}")"; \
        ln -sf /usr/bin/java "${JUDGE_JAVA_RUNTIME_PATH}"; \
    fi
CMD ["./judge_server"]
