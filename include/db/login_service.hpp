#pragma once
#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace login_service{
    struct login_session{
        std::int64_t user_id = 0;
        bool is_admin = false;
        std::string token;
    };

    std::expected<login_session, error_code> sign_up(
        db_connection& connection_value,
        std::string_view user_login_id,
        std::string_view raw_password
    );

    std::expected<std::optional<login_session>, error_code> login(
        db_connection& connection_value,
        std::string_view user_login_id,
        std::string_view raw_password
    );
}
