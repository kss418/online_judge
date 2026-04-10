#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace system_dto{
    struct http_db_pool{
        std::int64_t size = 0;
        std::int64_t available_count = 0;
    };

    struct http_handler_queue{
        std::int64_t backlog_count = 0;
        std::optional<std::int64_t> limit_opt = std::nullopt;
    };

    struct http_runtime_snapshot{
        http_db_pool db_pool;
        http_handler_queue handler_queue;
    };

    struct snapshot_cache_metrics{
        std::int64_t hit_count = 0;
        std::int64_t miss_count = 0;
    };

    struct sandbox_self_check{
        std::string status = "unknown";
        std::optional<std::string> checked_at_opt = std::nullopt;
        std::optional<std::string> message_opt = std::nullopt;
    };

    struct judge_status_snapshot{
        std::int64_t queue_depth = 0;
        std::int64_t live_instance_count = 0;
        std::int64_t configured_worker_count = 0;
        std::int64_t active_worker_count = 0;
        snapshot_cache_metrics snapshot_cache;
        sandbox_self_check last_sandbox_self_check;
    };

    struct status_response{
        http_runtime_snapshot http;
        judge_status_snapshot judge;
    };
}
