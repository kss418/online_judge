#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace problem_core_service{
    std::expected<bool, error_code> exists_problem(
        db_connection& connection,
        std::int64_t problem_id
    );
    
    std::expected<std::int32_t, error_code> get_version(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<void, error_code> increase_version(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<std::int64_t, error_code> create_problem(db_connection& connection);
    std::expected<limits, error_code> get_limits(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<void, error_code> set_limits(
        db_connection& connection,
        std::int64_t problem_id,
        const limits& limits_value
    );
}
