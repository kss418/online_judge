#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_request_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <vector>

namespace get_submission_status_batch_query{
    using command = submission_request_dto::status_batch_request;

    std::expected<std::vector<submission_response_dto::status_snapshot>, service_error>
    execute(
        db_connection& connection,
        const command& command_value
    );
}
