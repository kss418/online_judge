#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_request_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <optional>

namespace list_submissions_query{
    struct command{
        submission_request_dto::list_filter filter_value;
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    };

    std::expected<submission_response_dto::summary_page, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
