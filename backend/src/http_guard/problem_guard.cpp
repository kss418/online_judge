#include "http_guard/problem_guard.hpp"

#include "db_service/problem_core_service.hpp"

std::expected<void, problem_guard::response_type> problem_guard::require_existing_problem_or_response(
    const request_type& request,
    db_connection& db_connection,
    const problem_dto::reference& problem_reference_value
){
    const auto exists_problem_exp = problem_core_service::exists_problem(
        db_connection,
        problem_reference_value
    );
    if(!exists_problem_exp){
        return std::unexpected(http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to check problem: " + to_string(exists_problem_exp.error())
        ));
    }
    if(!exists_problem_exp->exists){
        return std::unexpected(http_response_util::create_problem_not_found(request));
    }

    return {};
}

std::expected<void, problem_guard::response_type> problem_guard::require_exists(
    const request_type& request,
    db_connection& db_connection,
    const problem_dto::reference& problem_reference_value
){
    return require_existing_problem_or_response(
        request,
        db_connection,
        problem_reference_value
    );
}
