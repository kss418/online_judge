#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_internal_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/service_error.hpp"

#include <cstdint>
#include <expected>

namespace submission_command_service{
    std::expected<submission_response_dto::queued_response, service_error> create_submission(
        db_connection& connection,
        const submission_internal_dto::create_submission_command& create_request_value
    );
    std::expected<submission_response_dto::queued_response, service_error> rejudge_submission(
        db_connection& connection,
        std::int64_t submission_id
    );
    std::expected<void, service_error> rejudge_problem(
        db_connection& connection,
        std::int64_t problem_id
    );
}
