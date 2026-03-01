FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        bash \
        build-essential \
        ca-certificates \
        cmake \
        curl \
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
COPY --from=build /workspace/judge_server /app/judge_server
CMD ["./judge_server"]
