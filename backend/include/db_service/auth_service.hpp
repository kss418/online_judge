#pragma once
#include "error/error_code.hpp"
#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace auth_service{
    std::expected<std::optional<auth_dto::identity>, error_code> auth_token(
        db_connection& connection_value,
        const auth_dto::token& token_value
    );
    std::expected<bool, error_code> renew_token(
        db_connection& connection_value,
        const auth_dto::token& token_value
    );
    std::expected<bool, error_code> revoke_token(
        db_connection& connection_value,
        const auth_dto::token& token_value
    );
    std::expected<bool, error_code> update_permission_level(
        db_connection& connection_value,
        std::int64_t user_id,
        std::int32_t permission_level
    );
    std::expected<auth_dto::user_summary_list, error_code> get_user_list(
        db_connection& connection_value
    );
}
