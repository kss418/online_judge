#pragma once

#include "common/db_connection.hpp"
#include "dto/system_dto.hpp"
#include "error/service_error.hpp"

#include <chrono>
#include <expected>

namespace system_service{
    std::expected<system_dto::status_response, service_error> get_status(
        db_connection& connection,
        const system_dto::http_runtime_snapshot& http_runtime_snapshot,
        std::chrono::milliseconds judge_heartbeat_stale_after
    );
}
