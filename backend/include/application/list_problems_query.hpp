#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace list_problems_query{
    struct command{
        problem_dto::list_filter filter_value;
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    };

    struct result{
        std::vector<problem_dto::summary> summary_values;
        std::int64_t total_problem_count = 0;
    };

    std::expected<result, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
