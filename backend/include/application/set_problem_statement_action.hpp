#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace set_problem_statement_action{
    struct command{
        problem_dto::reference problem_reference_value;
        problem_content_dto::statement statement_value;
    };

    std::expected<problem_dto::mutation_result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
