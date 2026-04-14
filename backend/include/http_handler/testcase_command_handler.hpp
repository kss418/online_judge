#pragma once

#include "http_handler/testcase_bulk_command_handler.hpp"
#include "http_handler/testcase_item_command_handler.hpp"
#include "http_handler/testcase_move_command_handler.hpp"

namespace testcase_command_handler{
    // Aggregated endpoint declaration header kept for compatibility.
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;
}
