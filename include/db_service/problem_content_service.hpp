#pragma once

#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace problem_content_service{
    std::expected<problem_dto::statement, error_code> get_statement(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<void, error_code> set_statement(
        db_connection& connection,
        std::int64_t problem_id,
        const problem_dto::statement& statement
    );

    std::expected<std::vector<problem_dto::sample>, error_code> list_samples(
        db_connection& connection,
        std::int64_t problem_id
    );
}
