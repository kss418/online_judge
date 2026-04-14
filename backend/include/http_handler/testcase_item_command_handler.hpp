#pragma once

#include "http_core/request_context.hpp"

#include <cstdint>

namespace testcase_command_handler{
    request_context::response_type post_testcase(
        request_context& context,
        std::int64_t problem_id
    );
    request_context::response_type put_testcase(
        request_context& context,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
    request_context::response_type delete_testcase(
        request_context& context,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
}
