#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace create_problem_sample_action{
    struct command{
        problem_dto::reference problem_reference_value;
    };

    std::expected<problem_dto::sample_mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace update_problem_sample_action{
    struct command{
        problem_content_dto::sample_ref sample_reference_value;
        problem_content_dto::sample sample_value;
    };

    std::expected<problem_dto::sample_mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace delete_problem_sample_action{
    using command = problem_dto::reference;

    std::expected<problem_dto::mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
