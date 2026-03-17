#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <cstdint>
#include <expected>
#include <string_view>

namespace submission_core_service{
    std::expected<std::int64_t, error_code> create_submission(
        db_connection& connection,
        std::int64_t user_id,
        std::int64_t problem_id,
        std::string_view language,
        std::string_view source_code
    );
}
