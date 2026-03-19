#pragma once

#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace problem_statistics_service{
    std::expected<problem_dto::statistics, error_code> get_statistics(
        db_connection& connection,
        std::int64_t problem_id
    );
}
