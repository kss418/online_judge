#pragma once

#include "http_core/request_context.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>

namespace problem_content_query_handler{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type get_limits(
        context_type& context,
        std::int64_t problem_id
    );
    response_type get_samples(
        context_type& context,
        std::int64_t problem_id
    );
}
