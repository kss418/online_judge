#pragma once

#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace sign_up_action{
    using command = auth_dto::sign_up_request;

    std::expected<auth_dto::session, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace login_action{
    using command = auth_dto::credentials;

    std::expected<auth_dto::session, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace renew_auth_token_action{
    using command = auth_dto::token;

    std::expected<void, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}

namespace logout_action{
    using command = auth_dto::token;

    std::expected<void, service_error> execute(
        db_connection& connection,
        const command& command_value
    );
}
