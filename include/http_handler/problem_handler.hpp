#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>
#include <string_view>

class problem_handler{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    problem_handler(const problem_handler&) = delete;
    problem_handler& operator=(const problem_handler&) = delete;
    problem_handler(problem_handler&&) noexcept = delete;
    problem_handler& operator=(problem_handler&&) noexcept = delete;

    explicit problem_handler(db_connection& db_connection);
    static bool is_problem_path(std::string_view path);
    response_type handle(const request_type& request, std::string_view path);
    response_type handle_create_problem_post(const request_type& request);
    response_type handle_set_limits_put(
        const request_type& request,
        std::int64_t problem_id
    );

private:
    static constexpr std::string_view path_prefix_ = "/api/problem";

    db_connection& db_connection_;
};
