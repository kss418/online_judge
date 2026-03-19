#pragma once

#include "db/db_connection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

namespace submission_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type handle_create_submission_post(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type handle_list_submissions_get(
        const request_type& request,
        db_connection& db_connection_value
    );
}
