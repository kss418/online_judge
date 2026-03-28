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
}
