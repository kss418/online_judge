#pragma once

#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace problem_core_service{
    inline constexpr std::int32_t INITIAL_MEMORY_LIMIT_MB = 256;
    inline constexpr std::int32_t INITIAL_TIME_LIMIT_MS = 1000;

    std::expected<problem_dto::existence, error_code> exists_problem(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::title, error_code> get_title(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
    
    std::expected<problem_dto::version, error_code> get_version(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::created, error_code> create_problem(
        db_connection& connection,
        const problem_dto::create_request& create_request_value
    );
    std::expected<problem_dto::limits, error_code> get_limits(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> set_limits(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::limits& limits_value
    );
}
