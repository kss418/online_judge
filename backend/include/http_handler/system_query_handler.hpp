#pragma once

#include "http_core/request_context.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

namespace system_query_handler{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type get_health(context_type& context);
    response_type get_supported_languages(context_type& context);
    response_type get_status(context_type& context);
}
