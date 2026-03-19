#pragma once

#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace testcase_service{
    std::expected<problem_dto::testcase, error_code> create_testcase(
        db_connection& connection,
        std::int64_t problem_id,
        const problem_dto::testcase& testcase_value
    );

    std::expected<problem_dto::testcase, error_code> get_testcase(
        db_connection& connection,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
    std::expected<std::int32_t, error_code> get_testcase_count(
        db_connection& connection,
        std::int64_t problem_id
    );
}
