#include "http_guard/problem_guard.hpp"

#include "db_service/problem_core_service.hpp"
#include "http_guard/auth_guard.hpp"

std::expected<void, problem_guard::response_type> problem_guard::require_exists(
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

std::expected<problem_dto::detail, problem_guard::response_type>
problem_guard::require_readable_detail(
    const request_type& request,
    db_connection& db_connection,
    const problem_dto::reference& problem_reference_value
){
    return http_guard::run_composite_guard(
        request,
        db_connection,
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
                if(problem_detail_exp.error() == http_error::not_found){
                    return std::unexpected(
                        http_response_util::create_problem_not_found(context.request)
                    );
                }

                return std::unexpected(http_response_util::create_404_or_500(
                    context.request,
                    "get problem detail",
                    problem_detail_exp.error()
                ));
            }

            return std::move(*problem_detail_exp);
        },
        auth_guard::make_optional_auth_guard()
    );
}
