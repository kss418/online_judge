#include "http_handler/problem_handler.hpp"
#include "dto/problem_dto.hpp"
#include "http_server/json_util.hpp"
#include "http_server/http_util.hpp"

#include "db_service/problem_content_service.hpp"
#include "db_service/problem_core_service.hpp"
#include "db_service/problem_statistics_service.hpp"

problem_handler::response_type problem_handler::get_problems(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto filter_exp = http_util::parse_problem_list_filter_or_400(request);
    if(!filter_exp){
        return std::move(filter_exp.error());
    }

    const auto summary_values_exp = problem_core_service::list_problems(
        db_connection_value,
        *filter_exp
    );
    if(!summary_values_exp){
        return http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to list problems: " + to_string(summary_values_exp.error())
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_problem_list_object(*summary_values_exp)
    );
}

problem_handler::response_type problem_handler::get_problem(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto exists_problem_exp = problem_core_service::exists_problem(
        db_connection_value,
        problem_reference_value
    );
    if(!exists_problem_exp){
        return http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to check problem: " + to_string(exists_problem_exp.error())
        );
    }
    if(!exists_problem_exp->exists){
        return http_response_util::create_error(
            request,
            boost::beast::http::status::not_found,
            "problem_not_found",
            "problem not found"
        );
    }

    const auto title_exp = problem_core_service::get_title(
        db_connection_value,
        problem_reference_value
    );
    if(!title_exp){
        return http_response_util::create_404_or_500(
            request,
            "get problem title",
            title_exp.error()
        );
    }

    const auto version_exp = problem_core_service::get_version(
        db_connection_value,
        problem_reference_value
    );
    if(!version_exp){
        return http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to get problem version: " + to_string(version_exp.error())
        );
    }

    const auto limits_exp = problem_core_service::get_limits(
        db_connection_value,
        problem_reference_value
    );
    if(!limits_exp){
        return http_response_util::create_404_or_500(
            request,
            "get problem limits",
            limits_exp.error()
        );
    }

    std::optional<problem_dto::statement> statement_opt = std::nullopt;
    const auto statement_exp = problem_content_service::get_statement(
        db_connection_value,
        problem_reference_value
    );
    if(statement_exp){
        statement_opt = statement_exp.value();
    }
    else if(statement_exp.error() != errno_error::invalid_argument){
        return http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to get problem statement: " + to_string(statement_exp.error())
        );
    }

    const auto samples_exp = problem_content_service::list_samples(
        db_connection_value,
        problem_reference_value
    );
    if(!samples_exp){
        return http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to list problem samples: " + to_string(samples_exp.error())
        );
    }

    const auto statistics_exp = problem_statistics_service::get_statistics(
        db_connection_value,
        problem_reference_value
    );
    if(!statistics_exp){
        return http_response_util::create_404_or_500(
            request,
            "get problem statistics",
            statistics_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_problem_detail_object(
            problem_reference_value,
            *title_exp,
            *version_exp,
            *limits_exp,
            statement_opt,
            *samples_exp,
            *statistics_exp
        )
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
        if(!create_problem_exp){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::internal_server_error,
                "internal_server_error",
                "failed to create problem: " + to_string(create_problem_exp.error())
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::created,
            json_util::make_problem_created_object(*create_problem_exp)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}
