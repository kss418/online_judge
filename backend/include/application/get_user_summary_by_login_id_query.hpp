#pragma once

#include "common/db_connection.hpp"
#include "dto/user_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <string>

namespace get_user_summary_by_login_id_query{
    struct command{
        std::string user_login_id;
    };

    std::expected<user_dto::summary, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
