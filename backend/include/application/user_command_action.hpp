#pragma once

#include "common/db_connection.hpp"
#include "error/service_error.hpp"
#include "dto/user_dto.hpp"

#include <expected>

namespace update_user_permission_action{
    struct command{
        std::int64_t user_id = 0;
        std::int32_t permission_level = 0;
    };

    std::expected<void, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace create_user_submission_ban_action{
    struct command{
        std::int64_t user_id = 0;
        std::int32_t duration_minutes = 0;
    };

    std::expected<user_dto::submission_ban, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace clear_user_submission_ban_action{
    struct command{
        std::int64_t user_id = 0;
    };

    std::expected<void, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
