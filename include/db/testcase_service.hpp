#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace testcase_service{
    std::expected<std::int64_t, error_code> create_testcase(
        db_connection& connection,
        std::int64_t problem_id,
        const testcase& testcase_value
    );

    std::expected<testcase, error_code> get_testcase(
        db_connection& connection,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
    std::expected<std::int32_t, error_code> get_testcase_count(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<std::vector<testcase>, error_code> list_testcases(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<void, error_code> set_testcase(
        db_connection& connection,
        std::int64_t problem_id,
        const testcase& testcase_value
    );

    std::expected<void, error_code> delete_testcase(
        db_connection& connection,
        std::int64_t problem_id
    );
}
