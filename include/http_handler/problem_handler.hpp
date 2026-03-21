#pragma once

#include "common/db_connection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>

namespace problem_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type handle_list_problems_get(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type handle_get_problem_get(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type handle_create_problem_post(
        const request_type& request,
        db_connection& db_connection_value
    );
    response_type handle_list_testcases_get(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
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
    response_type handle_create_testcase_post(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type handle_set_testcase_put(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
    response_type handle_delete_testcase_delete(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
}
