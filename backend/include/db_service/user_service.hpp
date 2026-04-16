#pragma once

#include "common/db_connection.hpp"
#include "error/service_error.hpp"
#include "dto/user_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace user_service{
    std::expected<user_dto::list, service_error> get_public_list(
        db_connection& connection,
        const user_dto::list_filter& filter_value
    );
    std::expected<void, service_error> ensure_user_exists(
        db_connection& connection,
        std::int64_t user_id
    );
    std::expected<user_dto::summary, service_error> get_summary(
        db_connection& connection,
        std::int64_t user_id
    );
    std::expected<user_dto::summary, service_error> get_summary_by_login_id(
        db_connection& connection,
        std::string_view user_login_id
    );
    std::expected<user_dto::submission_ban, service_error> create_submission_ban(
        db_connection& connection,
        std::int64_t user_id,
        std::int32_t duration_minutes
    );
    std::expected<user_dto::submission_ban_status, service_error>
    get_submission_ban_status(
        db_connection& connection,
        std::int64_t user_id
    );
    std::expected<void, service_error> clear_submission_banned_until(
        db_connection& connection,
        std::int64_t user_id
    );
}
