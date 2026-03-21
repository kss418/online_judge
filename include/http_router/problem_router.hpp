#pragma once

#include "common/db_connection.hpp"
#include "http_handler/problem_handler.hpp"

#include <cstdint>
#include <string_view>

class problem_router{
public:
    using request_type = problem_handler::request_type;
    using response_type = problem_handler::response_type;

    problem_router(const problem_router&) = delete;
    problem_router& operator=(const problem_router&) = delete;
    problem_router(problem_router&&) noexcept = delete;
    problem_router& operator=(problem_router&&) noexcept = delete;

    explicit problem_router(db_connection& db_connection);
    response_type route(const request_type& request, std::string_view path);

private:
    response_type handle_problems(const request_type& request);
    response_type handle_get_problem(
        const request_type& request,
        std::int64_t problem_id
    );
    response_type handle_set_limits(
        const request_type& request,
        std::int64_t problem_id
    );
    response_type handle_set_statement(
        const request_type& request,
        std::int64_t problem_id
    );
    response_type handle_testcases(
        const request_type& request,
        std::int64_t problem_id
    );
    response_type handle_testcase(
        const request_type& request,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );

    db_connection& db_connection_;
};
