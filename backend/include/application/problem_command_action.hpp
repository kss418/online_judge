#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace create_problem_action{
    using command = problem_dto::create_request;

    std::expected<problem_dto::created, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace update_problem_action{
    struct command{
        problem_dto::reference problem_reference_value;
        problem_dto::update_request update_request_value;
    };

    std::expected<problem_dto::mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace delete_problem_action{
    using command = problem_dto::reference;

    std::expected<void, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace rejudge_problem_action{
    struct command{
        std::int64_t problem_id = 0;
    };

    std::expected<void, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
