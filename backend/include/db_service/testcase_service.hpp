#pragma once

#include "error/service_error.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace testcase_service{
    std::expected<problem_dto::testcase_mutation_result, service_error> create_testcase(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::testcase& testcase_value
    );

    std::expected<problem_dto::testcase, service_error> get_testcase(
        db_connection& connection,
        const problem_dto::testcase_ref& testcase_reference_value
    );
    std::expected<problem_dto::testcase_count, service_error> get_testcase_count(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<std::vector<problem_dto::testcase>, service_error> list_testcases(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<std::vector<problem_dto::testcase_summary>, service_error> list_testcase_summaries(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<void, service_error> set_testcase(
        db_connection& connection,
        const problem_dto::testcase_ref& testcase_reference_value,
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

    std::expected<problem_dto::mutation_result, service_error> delete_all_testcases(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::testcase_count_mutation_result, service_error> replace_testcases(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const std::vector<problem_dto::testcase>& testcase_values
    );
}
