#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <vector>

namespace replace_testcases_action{
    struct command{
        problem_dto::reference problem_reference_value;
        std::vector<problem_dto::testcase> testcase_values;
    };

    std::expected<problem_dto::testcase_count, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
