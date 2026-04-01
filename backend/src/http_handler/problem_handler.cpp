#include "http_handler/problem_handler.hpp"
#include "dto/problem_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_core/request_dto.hpp"

#include "db_service/problem_core_service.hpp"
#include "db_service/submission_service.hpp"
#include "serializer/problem_json_serializer.hpp"

problem_handler::response_type problem_handler::get_problems(
    const request_type& request,
    db_connection& db_connection_value
){
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const std::optional<auth_dto::identity>& auth_identity_opt,
            const problem_dto::list_filter& filter_value) -> response_type {
            std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
            if(auth_identity_opt.has_value()){
                viewer_user_id_opt = auth_identity_opt->user_id;
            }

            if(filter_value.state_opt && !viewer_user_id_opt){
                return http_response_util::create_error(
                    request,
                    boost::beast::http::status::bad_request,
                    "invalid_query_parameter",
                    "invalid query parameter: state",
                    "state"
                );
            }

            const auto summary_values_exp = problem_core_service::list_problems(
                db_connection_value,
                filter_value,
                viewer_user_id_opt
            );
            if(!summary_values_exp){
                return http_response_util::create_error(
                    request,
                    boost::beast::http::status::internal_server_error,
                    "internal_server_error",
                    "failed to list problems: " + to_string(summary_values_exp.error())
                );
            }

            const auto total_problem_count_exp = problem_core_service::count_problems(
                db_connection_value,
                filter_value,
                viewer_user_id_opt
            );
            if(!total_problem_count_exp){
                return http_response_util::create_error(
                    request,
                    boost::beast::http::status::internal_server_error,
                    "internal_server_error",
                    "failed to count problems: " + to_string(total_problem_count_exp.error())
                );
            }

            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                problem_json_serializer::make_list_object(
                    *summary_values_exp,
                    *total_problem_count_exp
                )
            );
        },
        auth_guard::make_optional_auth_guard(),
        request_dto::make_problem_list_filter_guard()
    );
}

problem_handler::response_type problem_handler::get_problem(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    const auto auth_identity_opt_exp = auth_guard::require_optional_auth(
        request,
        db_connection_value
    );
    if(!auth_identity_opt_exp){
        return std::move(auth_identity_opt_exp.error());
    }

    problem_dto::reference problem_reference_value{problem_id};
    std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    if(auth_identity_opt_exp->has_value()){
        viewer_user_id_opt = auth_identity_opt_exp->value().user_id;
    }

    const auto problem_detail_exp = problem_core_service::get_problem_detail(
        db_connection_value,
        problem_reference_value,
        viewer_user_id_opt
    );
    return http_response_util::create_response_or_error(
        request,
        "get problem detail",
        std::move(problem_detail_exp),
        [&](const request_type& error_request, std::string_view action, const error_code& code) {
            if(code == errno_error::invalid_argument){
                return http_response_util::create_problem_not_found(error_request);
            }

            return http_response_util::create_404_or_500(
                error_request,
                action,
                code
            );
        },
        [&](const problem_dto::detail& problem_detail) {
            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                problem_json_serializer::make_detail_object(problem_detail)
            );
        }
    );
}

problem_handler::response_type problem_handler::post_problem(
    const request_type& request,
    db_connection& db_connection_value
){
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const auth_dto::identity&, const problem_dto::create_request& create_request) {
            const auto create_problem_exp = problem_core_service::create_problem(
                db_connection_value,
                create_request
            );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "create problem",
                std::move(create_problem_exp),
                problem_json_serializer::make_created_object,
                boost::beast::http::status::created
            );
        },
        auth_guard::make_admin_guard(),
        request_dto::make_json_guard<problem_dto::create_request>(
            problem_dto::make_create_request_from_json
        )
    );
}

problem_handler::response_type problem_handler::put_problem(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const auth_dto::identity&, const problem_dto::update_request& update_request) {
            const auto update_problem_exp = problem_core_service::update_problem(
                db_connection_value,
                problem_reference_value,
                update_request
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "update problem",
                std::move(update_problem_exp),
                "problem updated"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value),
        request_dto::make_json_guard<problem_dto::update_request>(
            problem_dto::make_update_request_from_json
        )
    );
}

problem_handler::response_type problem_handler::delete_problem(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const auth_dto::identity&) {
            const auto delete_problem_exp = problem_core_service::delete_problem(
                db_connection_value,
                problem_reference_value
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "delete problem",
                std::move(delete_problem_exp),
                "problem deleted"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

problem_handler::response_type problem_handler::post_problem_rejudge(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const auth_dto::identity&) {
            const auto rejudge_problem_exp = submission_service::rejudge_problem(
                db_connection_value,
                problem_id
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "rejudge problem",
                std::move(rejudge_problem_exp),
                "problem submissions requeued"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}
