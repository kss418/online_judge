#pragma once

#include "common/db_connection.hpp"
#include "dto/user_dto.hpp"
#include "http_core/http_response_util.hpp"

#include <cstdint>
#include <expected>
#include <string_view>

namespace user_guard{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    std::expected<user_dto::summary, response_type> require_summary(
        const request_type& request,
        db_connection& db_connection,
        std::int64_t user_id
    );
    std::expected<user_dto::summary, response_type> require_summary_by_login_id(
        const request_type& request,
        db_connection& db_connection,
        std::string_view user_login_id
    );
}
