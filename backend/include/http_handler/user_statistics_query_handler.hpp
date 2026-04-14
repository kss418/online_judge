#pragma once

#include "http_core/request_context.hpp"

#include <cstdint>

namespace user_query_handler{
    request_context::response_type get_me_submission_statistics(
        request_context& context
    );
    request_context::response_type get_user_submission_statistics(
        request_context& context,
        std::int64_t user_id
    );
}
