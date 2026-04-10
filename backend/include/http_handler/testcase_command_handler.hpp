#pragma once

#include "http_core/request_context.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>

namespace testcase_command_handler{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type post_testcase(
        context_type& context,
        std::int64_t problem_id
    );
    response_type put_testcase(
        context_type& context,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
    response_type post_testcase_zip(
        context_type& context,
        std::int64_t problem_id
    );
    response_type move_testcase(
        context_type& context,
        std::int64_t problem_id
    );
    response_type delete_testcase(
        context_type& context,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
    response_type delete_all_testcases(
        context_type& context,
        std::int64_t problem_id
    );
}
