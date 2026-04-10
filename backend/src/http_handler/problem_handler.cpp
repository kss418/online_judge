#include "http_handler/problem_handler.hpp"
#include "application/get_problem_detail_query.hpp"
#include "application/list_problems_query.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_guard.hpp"

#include "db_service/problem_core_service.hpp"
#include "db_service/submission_service.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

problem_handler::response_type problem_handler::get_problems(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const std::optional<auth_dto::identity>& auth_identity_opt,
            const problem_dto::list_filter& filter_value) -> response_type {
            const auto viewer_user_id_opt = auth_guard::get_viewer_user_id(
                auth_identity_opt
            );

            list_problems_query::command command_value{
                .filter_value = filter_value,
                .viewer_user_id_opt = viewer_user_id_opt
            };
            const auto filter_validation_exp =
                problem_request_parser::validate_list_filter_for_viewer(
                    command_value.filter_value,
                    viewer_user_id_opt.has_value()
                );
            if(!filter_validation_exp){
                return http_adapter::error(
                    context_value.request,
                    filter_validation_exp.error()
                );
            }

            const auto result_exp = list_problems_query::execute(
                context_value.db_connection_ref(),
                command_value
            );
            return http_adapter::json(
                context_value.request,
                std::move(result_exp),
                [](const list_problems_query::result& result_value) {
                    return problem_json_serializer::make_list_object(
                        result_value.summary_values,
                        result_value.total_problem_count
                    );
                }
            );
        },
        auth_guard::make_optional_auth_guard(),
        request_guard::make_problem_list_filter_guard()
    );
}

problem_handler::response_type problem_handler::get_problem(
    context_type& context,
    std::int64_t problem_id
){
    return http_guard::run_or_respond(
        context,
        [problem_id](context_type& context_value,
            const std::optional<auth_dto::identity>& auth_identity_opt) -> response_type {
            get_problem_detail_query::command command_value{
                .problem_reference_value = problem_dto::reference{problem_id},
                .viewer_user_id_opt = auth_guard::get_viewer_user_id(auth_identity_opt)
            };
            return http_adapter::json(
                context_value.request,
                get_problem_detail_query::execute(
                    context_value.db_connection_ref(),
                    command_value
                ),
                [](const problem_dto::detail& problem_detail) {
                    return problem_json_serializer::make_detail_object(problem_detail);
                }
            );
        },
        auth_guard::make_optional_auth_guard()
    );
}

problem_handler::response_type problem_handler::post_problem(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const auth_dto::identity&,
            const problem_dto::create_request& create_request) -> response_type {
            const auto create_problem_exp = problem_core_service::create_problem(
                context_value.db_connection_ref(),
                create_request
            );
            return http_adapter::json(
                context_value.request,
                std::move(create_problem_exp),
                problem_json_serializer::make_created_object,
                boost::beast::http::status::created
            );
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<problem_dto::create_request>(
            problem_request_parser::parse_create_request
        )
    );
}

problem_handler::response_type problem_handler::put_problem(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_reference_value](context_type& context_value,
            const auth_dto::identity&,
            const problem_dto::update_request& update_request) -> response_type {
            const auto update_problem_exp = problem_core_service::update_problem(
                context_value.db_connection_ref(),
                problem_reference_value,
                update_request
            );
            return http_adapter::message(
                context_value.request,
                std::move(update_problem_exp),
                "problem updated"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value),
        request_guard::make_json_guard<problem_dto::update_request>(
            problem_request_parser::parse_update_request
        )
    );
}

problem_handler::response_type problem_handler::delete_problem(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_reference_value](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const auto delete_problem_exp = problem_core_service::delete_problem(
                context_value.db_connection_ref(),
                problem_reference_value
            );
            return http_adapter::message(
                context_value.request,
                std::move(delete_problem_exp),
                "problem deleted"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

problem_handler::response_type problem_handler::post_problem_rejudge(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_id](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const auto rejudge_problem_exp = submission_service::rejudge_problem(
                context_value.db_connection_ref(),
                problem_id
            );
            return http_adapter::message(
                context_value.request,
                std::move(rejudge_problem_exp),
                "problem submissions requeued"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}
