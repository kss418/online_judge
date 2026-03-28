#pragma once

#include "common/db_connection.hpp"
#include "common/error_code.hpp"
#include "dto/user_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace user_service{
    std::expected<std::optional<user_dto::summary>, error_code> get_summary(
        db_connection& connection,
        std::int64_t user_id
    );
    std::expected<std::optional<user_dto::summary>, error_code> get_summary_by_login_id(
        db_connection& connection,
        std::string_view user_login_id
    );
}
