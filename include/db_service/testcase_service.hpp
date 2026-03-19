#pragma once

#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace testcase_service{
    std::expected<problem_dto::testcase, error_code> create_testcase(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::testcase& testcase_value
    );

    std::expected<problem_dto::testcase, error_code> get_testcase(
        db_connection& connection,
        const problem_dto::testcase_ref& testcase_reference_value
    );
    std::expected<problem_dto::testcase_count, error_code> get_testcase_count(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
}
