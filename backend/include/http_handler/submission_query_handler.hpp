#pragma once

#include "http_core/request_context.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>

namespace submission_query_handler{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    response_type get_submission_history(context_type& context, std::int64_t submission_id);
    response_type get_submission_source(context_type& context, std::int64_t submission_id);
    response_type get_submission(context_type& context, std::int64_t submission_id);
    response_type post_submission_status_batch(context_type& context);
    response_type get_submissions(context_type& context);
}
