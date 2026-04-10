#pragma once

#include "error/infra_error.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>

struct http_runtime_config{
    std::uint16_t port = 0;
    std::size_t io_worker_count = 0;
    std::size_t handler_worker_count = 0;
    std::size_t db_pool_size = 0;
    std::optional<std::size_t> handler_queue_limit_opt;
    std::optional<std::chrono::milliseconds> db_acquire_timeout_opt;
    std::optional<std::chrono::milliseconds> request_deadline_opt;
    std::optional<std::chrono::milliseconds> keep_alive_idle_timeout_opt;

    static std::expected<http_runtime_config, infra_error> load(
        std::size_t default_http_worker_count
    );
};
