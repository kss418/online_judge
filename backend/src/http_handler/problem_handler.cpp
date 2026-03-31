#include "http_handler/problem_handler.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/http_util.hpp"

#include "db_service/problem_core_service.hpp"
#include "db_service/submission_service.hpp"
#include "serializer/problem_json_serializer.hpp"

problem_handler::response_type problem_handler::get_problems(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto auth_identity_opt_exp = http_util::try_optional_auth_bearer(
        request,
        db_connection_value
    );
    if(!auth_identity_opt_exp){
        return std::move(auth_identity_opt_exp.error());
    }

    const auto filter_exp = http_util::parse_problem_list_filter_or_400(request);
    if(!filter_exp){
        return std::move(filter_exp.error());
    }

    std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    if(auth_identity_opt_exp->has_value()){
        viewer_user_id_opt = auth_identity_opt_exp->value().user_id;
    }

    if(filter_exp->state_opt && !viewer_user_id_opt){
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
        *filter_exp,
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
        *filter_exp,
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
}

problem_handler::response_type problem_handler::get_problem(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    const auto auth_identity_opt_exp = http_util::try_optional_auth_bearer(
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
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto create_request_exp =
            http_util::parse_json_dto_or_400<problem_dto::create_request>(
                request,
                problem_dto::make_create_request_from_json
            );
        if(!create_request_exp){
            return std::move(create_request_exp.error());
        }

        const auto create_problem_exp = problem_core_service::create_problem(
            db_connection_value,
            *create_request_exp
        );
        return http_response_util::create_json_or_4xx_or_500(
            request,
            "create problem",
            std::move(create_problem_exp),
            problem_json_serializer::make_created_object,
            boost::beast::http::status::created
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

problem_handler::response_type problem_handler::put_problem(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto update_request_exp =
            http_util::parse_json_dto_or_400<problem_dto::update_request>(
                request,
                problem_dto::make_update_request_from_json
            );
        if(!update_request_exp){
            return std::move(update_request_exp.error());
        }

        const auto update_problem_exp = problem_core_service::update_problem(
            db_connection_value,
            problem_reference_value,
            *update_request_exp
        );
        return http_response_util::create_message_or_4xx_or_500(
            request,
            "update problem",
            std::move(update_problem_exp),
            "problem updated"
        );
    };

    return http_util::with_existing_problem_admin(
        request,
        db_connection_value,
        problem_reference_value,
        handle_authenticated
    );
}

problem_handler::response_type problem_handler::delete_problem(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
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
    };

    return http_util::with_existing_problem_admin(
        request,
        db_connection_value,
        problem_reference_value,
        handle_authenticated
    );
}

problem_handler::response_type problem_handler::post_problem_rejudge(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
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
    };

    return http_util::with_existing_problem_admin(
        request,
        db_connection_value,
        problem_reference_value,
        handle_authenticated
    );
}
