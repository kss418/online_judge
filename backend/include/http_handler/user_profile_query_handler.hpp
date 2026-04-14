#pragma once

#include "http_core/request_context.hpp"

#include <cstdint>
#include <string_view>

namespace user_query_handler{
    request_context::response_type get_me(request_context& context);
    request_context::response_type get_user_summary(
        request_context& context,
        std::int64_t user_id
    );
    request_context::response_type get_user_summary_by_login_id(
        request_context& context,
        std::string_view user_login_id
    );
}
