#pragma once

#include "http_core/request_context.hpp"

namespace user_query_handler{
    request_context::response_type get_public_user_list(request_context& context);
    request_context::response_type get_user_list(request_context& context);
}
