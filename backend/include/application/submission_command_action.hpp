#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace rejudge_submission_action{
    struct command{
        std::int64_t submission_id = 0;
    };

    std::expected<submission_response_dto::queued_response, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
