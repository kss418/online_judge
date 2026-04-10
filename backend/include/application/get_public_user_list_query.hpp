#pragma once

#include "common/db_connection.hpp"
#include "dto/user_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace get_public_user_list_query{
    struct command{
        user_dto::list_filter filter_value;
    };

    std::expected<user_dto::list, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
