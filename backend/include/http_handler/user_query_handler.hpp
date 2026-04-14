#pragma once

#include "http_handler/user_list_query_handler.hpp"
#include "http_handler/user_problem_list_query_handler.hpp"
#include "http_handler/user_profile_query_handler.hpp"
#include "http_handler/user_statistics_query_handler.hpp"
#include "http_handler/user_submission_ban_query_handler.hpp"

namespace user_query_handler{
    // Aggregated endpoint declaration header kept for compatibility.
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;
}
