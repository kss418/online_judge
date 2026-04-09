#include "http_guard/problem_guard.hpp"

#include "db_service/problem_core_service.hpp"
#include "http_guard/auth_guard.hpp"

std::expected<void, problem_guard::response_type> problem_guard::require_exists(
    context_type& context,
    const problem_dto::reference& problem_reference_value
){
    const auto ensure_problem_exists_exp = problem_core_service::ensure_problem_exists(
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

std::expected<problem_dto::detail, problem_guard::response_type>
problem_guard::require_readable_detail(
    context_type& context,
    const problem_dto::reference& problem_reference_value
){
    return http_guard::run_composite_guard(
        context,
        [problem_reference_value](const http_guard::guard_context& context,
            const std::optional<auth_dto::identity>& auth_identity_opt)
            -> std::expected<problem_dto::detail, response_type> {
            const auto viewer_user_id_opt = auth_guard::get_viewer_user_id(
                auth_identity_opt
            );

            auto problem_detail_exp = problem_core_service::get_problem_detail(
                context.db_connection_value,
                problem_reference_value,
                viewer_user_id_opt
            );
            if(!problem_detail_exp){
                return std::unexpected(http_response_util::create_4xx_or_500(
                    context.request,
                    problem_detail_exp.error()
                ));
            }

            return std::move(*problem_detail_exp);
        },
        auth_guard::make_optional_auth_guard()
    );
}
