#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace problem_core_service{
    std::expected<std::int32_t, error_code> increase_testcase_count(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    );

    std::expected<std::int32_t, error_code> decrease_testcase_count(
        pqxx::transaction_base& transaction,
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
