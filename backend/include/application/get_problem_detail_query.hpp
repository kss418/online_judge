#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <optional>

namespace get_problem_detail_query{
    struct command{
        problem_dto::reference problem_reference_value;
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    };

    std::expected<problem_dto::detail, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
