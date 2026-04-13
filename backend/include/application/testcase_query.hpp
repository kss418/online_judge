#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <vector>

namespace get_testcase_query{
    using command = problem_dto::testcase_ref;

    std::expected<problem_dto::testcase, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace list_testcase_summaries_query{
    using command = problem_dto::reference;

    std::expected<std::vector<problem_dto::testcase_summary>, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
