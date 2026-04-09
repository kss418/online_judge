#pragma once

#include "http_core/request_context.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>

namespace problem_content_handler{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type get_limits(
        context_type& context,
        std::int64_t problem_id
    );
    response_type put_limits(
        context_type& context,
        std::int64_t problem_id
    );
    response_type put_statement(
        context_type& context,
        std::int64_t problem_id
    );
    response_type get_samples(
        context_type& context,
        std::int64_t problem_id
    );
    response_type post_sample(
        context_type& context,
        std::int64_t problem_id
    );
    response_type put_sample(
        context_type& context,
        std::int64_t problem_id,
        std::int32_t sample_order
    );
    response_type delete_sample(
        context_type& context,
        std::int64_t problem_id
    );
}
