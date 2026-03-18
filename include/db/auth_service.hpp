#pragma once
#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "dto/auth_dto.hpp"

#include <expected>
#include <optional>
#include <string_view>

namespace auth_service{
    std::expected<std::optional<auth_dto::identity>, error_code> auth_token(
        db_connection& connection_value,
        std::string_view token
    );
    std::expected<bool, error_code> renew_token(
        db_connection& connection_value,
        std::string_view token
    );
    std::expected<bool, error_code> revoke_token(
        db_connection& connection_value,
        std::string_view token
    );
}
