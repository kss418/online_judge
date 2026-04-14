#pragma once

#include "http_core/request_context.hpp"

#include <cstdint>

namespace testcase_command_handler{
    request_context::response_type post_testcase_zip(
        request_context& context,
        std::int64_t problem_id
    );
    request_context::response_type delete_all_testcases(
        request_context& context,
        std::int64_t problem_id
    );
}
