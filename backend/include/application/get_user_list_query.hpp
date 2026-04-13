#pragma once

#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace get_user_list_query{
    struct command{};

    std::expected<auth_dto::user_summary_list, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
