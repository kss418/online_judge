#pragma once

#include "common/db_connection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

namespace submission_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type get_submission_source(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t submission_id
    );
    response_type get_submission(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t submission_id
    );
    response_type post_submission(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type get_submissions(
        const request_type& request,
        db_connection& db_connection_value
    );
}
