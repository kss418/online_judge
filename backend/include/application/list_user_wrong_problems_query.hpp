#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <optional>
#include <vector>

namespace list_user_wrong_problems_query{
    struct command{
        std::int64_t user_id = 0;
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    };

    std::expected<std::vector<problem_dto::summary>, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
