#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <vector>

namespace get_problem_limits_query{
    using command = problem_dto::reference;

    std::expected<problem_content_dto::limits, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace list_problem_samples_query{
    using command = problem_dto::reference;

    std::expected<std::vector<problem_content_dto::sample>, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
