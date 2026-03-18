#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "dto/submission_dto.hpp"

#include <cstdint>
#include <expected>

namespace submission_core_service{
    std::expected<std::int64_t, error_code> create_submission(
        db_connection& connection,
        std::int64_t user_id,
        std::int64_t problem_id,
        const submission_dto::source& source_value
    );
}
