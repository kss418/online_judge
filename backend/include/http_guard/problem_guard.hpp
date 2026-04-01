#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/http_response_util.hpp"
#include "http_guard/guard_runner.hpp"

#include <expected>

namespace problem_guard{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    std::expected<void, response_type> require_exists(
        const request_type& request,
        db_connection& db_connection,
        const problem_dto::reference& problem_reference_value
    );

    inline auto make_exists_guard(problem_dto::reference problem_reference_value){
        return [problem_reference_value](const http_guard::guard_context& context){
            return require_exists(
                context.request,
                context.db_connection_value,
                problem_reference_value
            );
        };
    }
}
