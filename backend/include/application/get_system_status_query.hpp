#pragma once

#include "common/db_connection.hpp"
#include "dto/system_dto.hpp"
#include "error/service_error.hpp"

#include <chrono>
#include <expected>

namespace get_system_status_query{
    struct command{
        system_dto::http_runtime_snapshot http_runtime_snapshot;
        std::chrono::milliseconds judge_heartbeat_stale_after{0};
    };

    std::expected<system_dto::status_response, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
