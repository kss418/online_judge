#pragma once

#include "common/db_connection.hpp"
#include "common/error_code.hpp"
#include "dto/user_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>

namespace user_service{
    std::expected<std::optional<user_dto::summary>, error_code> get_summary(
        db_connection& connection,
        std::int64_t user_id
    );
}
