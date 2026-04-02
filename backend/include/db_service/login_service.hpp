#pragma once
#include "error/service_error.hpp"
#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"

#include <expected>
#include <optional>

namespace login_service{
    std::expected<auth_dto::session, service_error> sign_up(
        db_connection& connection_value,
        const auth_dto::sign_up_request& sign_up_request_value
    );

    std::expected<std::optional<auth_dto::session>, service_error> login(
        db_connection& connection_value,
        const auth_dto::credentials& credentials_value
    );
}
