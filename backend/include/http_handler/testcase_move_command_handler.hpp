#pragma once

#include "http_core/request_context.hpp"

#include <cstdint>

namespace testcase_command_handler{
    request_context::response_type move_testcase(
        request_context& context,
        std::int64_t problem_id
    );
}
