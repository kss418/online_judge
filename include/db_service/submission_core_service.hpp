#pragma once

#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "dto/submission_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace submission_core_service{
    std::expected<std::int64_t, error_code> create_submission(
        db_connection& connection,
        std::int64_t user_id,
        std::int64_t problem_id,
        const submission_dto::source& source_value
    );
    std::expected<std::vector<submission_dto::summary>, error_code> list_submissions(
        db_connection& connection,
        const submission_dto::list_filter& filter_value
    );
}
