#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace tc_service{
    std::expected<problem_dto::tc, error_code> create_tc(
        db_connection& connection,
        std::int64_t problem_id,
        const problem_dto::tc& tc_value
    );

    std::expected<problem_dto::tc, error_code> get_tc(
        db_connection& connection,
        std::int64_t problem_id,
        std::int32_t tc_order
    );
    std::expected<std::int32_t, error_code> get_tc_count(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<std::vector<problem_dto::tc>, error_code> list_tcs(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<void, error_code> set_tc(
        db_connection& connection,
        std::int64_t problem_id,
        const problem_dto::tc& tc_value
    );

    std::expected<void, error_code> delete_tc(
        db_connection& connection,
        std::int64_t problem_id
    );
}
