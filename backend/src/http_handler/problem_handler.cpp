#include "http_handler/problem_handler.hpp"
#include "dto/problem_dto.hpp"
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

            const auto filter_validation_exp =
                problem_request_parser::validate_list_filter_for_viewer(
                    filter_value,
                    viewer_user_id_opt.has_value()
                );
            if(!filter_validation_exp){
                return http_response_util::create_error(
                    context_value.request,
                    filter_validation_exp.error()
                );
            }

            const auto summary_values_exp = problem_core_service::list_problems(
                context_value.db_connection_ref(),
                filter_value,
                viewer_user_id_opt
            );
            if(!summary_values_exp){
                return http_response_util::create_internal_server_error(
                    context_value.request,
                    "list_problems",
                    to_string(summary_values_exp.error())
                );
            }

            const auto total_problem_count_exp = problem_core_service::count_problems(
                context_value.db_connection_ref(),
                filter_value,
                viewer_user_id_opt
            );
            if(!total_problem_count_exp){
                return http_response_util::create_internal_server_error(
                    context_value.request,
                    "count_problems",
                    to_string(total_problem_count_exp.error())
                );
            }

            return http_response_util::create_json(
                context_value.request,
                boost::beast::http::status::ok,
                problem_json_serializer::make_list_object(
                    *summary_values_exp,
                    *total_problem_count_exp
                )
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
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const problem_dto::detail& problem_detail) -> response_type {
            return http_response_util::create_json(
                context_value.request,
                boost::beast::http::status::ok,
                problem_json_serializer::make_detail_object(problem_detail)
            );
        },
        problem_guard::make_readable_detail_guard(problem_reference_value)
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
            return http_response_util::create_json_or_4xx_or_500(
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
            return http_response_util::create_message_or_4xx_or_500(
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
            return http_response_util::create_message_or_4xx_or_500(
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
            return http_response_util::create_message_or_4xx_or_500(
                context_value.request,
                std::move(rejudge_problem_exp),
                "problem submissions requeued"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}
