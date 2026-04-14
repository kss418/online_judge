#pragma once

#include "http_core/request_context.hpp"

#include <cstdint>

namespace user_query_handler{
    request_context::response_type get_me_solved_problems(request_context& context);
    request_context::response_type get_me_wrong_problems(request_context& context);
    request_context::response_type get_user_solved_problems(
        request_context& context,
        std::int64_t user_id
    );
    request_context::response_type get_user_wrong_problems(
        request_context& context,
        std::int64_t user_id
    );
}
