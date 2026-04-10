#pragma once

#include "common/db_connection_pool.hpp"
#include "common/worker_pool.hpp"
#include "dto/system_dto.hpp"

#include <chrono>
#include <cstdint>

class http_runtime_status_provider{
public:
    http_runtime_status_provider(
        const db_connection_pool& db_connection_pool,
        const worker_pool& handler_worker_pool,
        std::chrono::milliseconds judge_heartbeat_stale_after
    ) :
        db_connection_pool_(db_connection_pool),
        handler_worker_pool_(handler_worker_pool),
        judge_heartbeat_stale_after_(judge_heartbeat_stale_after){}

    system_dto::http_runtime_snapshot snapshot() const{
        system_dto::http_runtime_snapshot snapshot_value;
        snapshot_value.db_pool.size =
            static_cast<std::int64_t>(db_connection_pool_.size());
        snapshot_value.db_pool.available_count =
            static_cast<std::int64_t>(db_connection_pool_.available_count());
        snapshot_value.handler_queue.backlog_count =
            static_cast<std::int64_t>(handler_worker_pool_.queued_task_count());
        if(const auto queue_limit_opt = handler_worker_pool_.queue_limit_opt()){
            snapshot_value.handler_queue.limit_opt =
                static_cast<std::int64_t>(*queue_limit_opt);
        }

        return snapshot_value;
    }

    std::chrono::milliseconds judge_heartbeat_stale_after() const noexcept{
        return judge_heartbeat_stale_after_;
    }

private:
    const db_connection_pool& db_connection_pool_;
    const worker_pool& handler_worker_pool_;
    std::chrono::milliseconds judge_heartbeat_stale_after_;
};
