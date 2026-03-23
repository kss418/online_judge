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

    response_type handle_set_limits_put(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type handle_set_statement_put(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type handle_list_samples_get(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type handle_create_sample_post(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type handle_set_sample_put(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id,
        std::int32_t sample_order
    );
    response_type handle_delete_sample_delete(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
}
