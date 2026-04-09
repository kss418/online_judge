#pragma once

#include "dto/problem_dto.hpp"
#include "http_core/request_context.hpp"
#include "http_guard/guard_runner.hpp"

#include <expected>

namespace problem_guard{
    using context_type = request_context;
    using request_type = context_type::request_type;
    using response_type = context_type::response_type;

    std::expected<void, response_type> require_exists(
        context_type& context,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<problem_dto::detail, response_type> require_readable_detail(
        context_type& context,
        const problem_dto::reference& problem_reference_value
    );

    inline auto make_exists_guard(problem_dto::reference problem_reference_value){
        return [problem_reference_value](const http_guard::guard_context& context){
            return require_exists(
                context.request_context_value,
                problem_reference_value
            );
        };
    }

    inline auto make_readable_detail_guard(problem_dto::reference problem_reference_value){
        return [problem_reference_value](const http_guard::guard_context& context){
            return require_readable_detail(
                context.request_context_value,
                problem_reference_value
            );
        };
    }
}
