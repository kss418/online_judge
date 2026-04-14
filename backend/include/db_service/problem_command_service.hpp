#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <cstdint>
#include <expected>

namespace problem_command_service{
    inline constexpr std::int32_t INITIAL_MEMORY_LIMIT_MB = 256;
    inline constexpr std::int32_t INITIAL_TIME_LIMIT_MS = 1000;

    std::expected<problem_dto::created, service_error> create_problem(
        db_connection& connection,
        const problem_dto::create_request& create_request_value
    );
    std::expected<problem_dto::mutation_result, service_error> update_problem(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::update_request& update_request_value
    );
    std::expected<void, service_error> delete_problem(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
}
