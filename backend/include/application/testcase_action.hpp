#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace create_testcase_action{
    struct command{
        problem_dto::reference problem_reference_value;
        problem_dto::testcase testcase_value;
    };

    std::expected<problem_dto::testcase_mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace update_testcase_action{
    struct command{
        problem_dto::testcase_ref testcase_reference_value;
        problem_dto::testcase testcase_value;
    };

    std::expected<problem_dto::testcase_mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace move_testcase_action{
    struct command{
        problem_dto::testcase_ref testcase_reference_value;
        std::int32_t target_testcase_order = 0;
    };

    std::expected<problem_dto::mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace delete_testcase_action{
    using command = problem_dto::testcase_ref;

    std::expected<problem_dto::mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace delete_all_testcases_action{
    using command = problem_dto::reference;

    std::expected<problem_dto::mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
