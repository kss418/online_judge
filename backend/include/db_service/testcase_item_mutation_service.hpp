#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <cstdint>
#include <expected>

namespace testcase_item_mutation_service{
    std::expected<problem_dto::testcase_mutation_result, service_error> create_testcase(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::testcase& testcase_value
    );
    std::expected<problem_dto::testcase_mutation_result, service_error> set_testcase_and_get(
        db_connection& connection,
        const problem_dto::testcase_ref& testcase_reference_value,
        const problem_dto::testcase& testcase_value
    );
    std::expected<problem_dto::mutation_result, service_error> move_testcase(
        db_connection& connection,
        const problem_dto::testcase_ref& testcase_reference_value,
        std::int32_t target_testcase_order
    );
    std::expected<problem_dto::mutation_result, service_error> delete_testcase(
        db_connection& connection,
        const problem_dto::testcase_ref& testcase_reference_value
    );
}
