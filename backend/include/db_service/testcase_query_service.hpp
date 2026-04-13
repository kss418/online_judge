#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <vector>

namespace testcase_query_service{
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
}
