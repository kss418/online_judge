#pragma once

#include "common/db_connection.hpp"
#include "dto/user_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace get_user_summary_query{
    struct command{
        std::int64_t user_id = 0;
    };

    std::expected<user_dto::summary, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
