#pragma once

#include "common/db_connection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>

namespace problem_content_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type put_limits(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type put_statement(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type get_samples(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type post_sample(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type put_sample(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id,
        std::int32_t sample_order
    );
    response_type delete_sample(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
}
