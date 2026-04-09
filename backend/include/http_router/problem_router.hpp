#pragma once

#include "http_handler/problem_handler.hpp"

#include <cstdint>
#include <string_view>

class problem_router{
public:
    using context_type = problem_handler::context_type;
    using request_type = problem_handler::request_type;
    using response_type = problem_handler::response_type;

    response_type route(context_type& context, std::string_view path);

private:
    response_type handle_problems(context_type& context);
    response_type handle_problem(
        context_type& context,
        std::int64_t problem_id
    );
    response_type handle_title(
        context_type& context,
        std::int64_t problem_id
    );
    response_type handle_set_limits(
        context_type& context,
        std::int64_t problem_id
    );
    response_type handle_statement(
        context_type& context,
        std::int64_t problem_id
    );
    response_type handle_samples(
        context_type& context,
        std::int64_t problem_id
    );
    response_type handle_sample(
        context_type& context,
        std::int64_t problem_id,
        std::int32_t sample_order
    );
    response_type handle_testcases(
        context_type& context,
        std::int64_t problem_id
    );
    response_type handle_testcase(
        context_type& context,
        std::int64_t problem_id,
        std::int32_t testcase_order
    );
    response_type handle_testcase_zip(
        context_type& context,
        std::int64_t problem_id
    );
    response_type handle_testcase_move(
        context_type& context,
        std::int64_t problem_id
    );
    response_type handle_testcase_all(
        context_type& context,
        std::int64_t problem_id
    );
    response_type handle_problem_rejudge(
        context_type& context,
        std::int64_t problem_id
    );
};
