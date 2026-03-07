#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <cstdint>
#include <expected>

namespace problem_statistics_service{
    std::expected<void, error_code> increase_submission_count(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<void, error_code> increase_accepted_count(
        db_connection& connection,
        std::int64_t problem_id
    );
}
