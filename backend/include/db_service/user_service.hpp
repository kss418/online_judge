#pragma once

#include "common/db_connection.hpp"
#include "common/error_code.hpp"
#include "dto/user_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace user_service{
    std::expected<user_dto::list, error_code> get_public_list(
        db_connection& connection,
        const user_dto::list_filter& filter_value
    );
    std::expected<std::optional<user_dto::summary>, error_code> get_summary(
        db_connection& connection,
        std::int64_t user_id
    );
    std::expected<std::optional<user_dto::summary>, error_code> get_summary_by_login_id(
        db_connection& connection,
        std::string_view user_login_id
    );
    std::expected<std::optional<user_dto::submission_ban>, error_code> create_submission_ban(
        db_connection& connection,
        std::int64_t user_id,
        std::int32_t duration_minutes
    );
    std::expected<std::optional<user_dto::submission_ban_status>, error_code>
    get_submission_ban_status(
        db_connection& connection,
        std::int64_t user_id
    );
    std::expected<bool, error_code> update_submission_banned_until(
        db_connection& connection,
        std::int64_t user_id,
        std::string_view submission_banned_until
    );
    std::expected<bool, error_code> clear_submission_banned_until(
        db_connection& connection,
        std::int64_t user_id
    );
}
