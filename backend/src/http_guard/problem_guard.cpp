#include "http_guard/problem_guard.hpp"

#include "db_service/problem_query_service.hpp"

std::expected<void, problem_guard::response_type> problem_guard::require_exists(
    context_type& context,
    const problem_dto::reference& problem_reference_value
){
    const auto ensure_problem_exists_exp = problem_query_service::ensure_problem_exists(
        context.db_connection_ref(),
        problem_reference_value
    );
    if(!ensure_problem_exists_exp){
        return std::unexpected(http_response_util::create_4xx_or_500(
            context.request,
            ensure_problem_exists_exp.error()
        ));
    }

    return {};
}
