#pragma once
#include "common/error_code.hpp"
#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"

#include <expected>
#include <optional>

namespace login_service{
    std::expected<auth_dto::session, error_code> sign_up(
        db_connection& connection_value,
        const auth_dto::credentials& credentials_value
    );

    std::expected<std::optional<auth_dto::session>, error_code> login(
        db_connection& connection_value,
        const auth_dto::credentials& credentials_value
    );
}
