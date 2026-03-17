#pragma once
#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace auth_service{
    struct auth_identity{
        std::int64_t user_id = 0;
        bool is_admin = false;
    };

    std::expected<std::optional<auth_identity>, error_code> auth_token(
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
