#pragma once

#include "common/db_connection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>

namespace testcase_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type get_testcases(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type post_testcase(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type put_testcase(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
    response_type post_testcase_zip(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
    response_type delete_testcase(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
    response_type delete_all_testcases(
        const request_type& request,
        db_connection& db_connection_value,
        std::int64_t problem_id
    );
}
