#include "http_handler/problem_handler.hpp"
#include "dto/problem_dto.hpp"
#include "http_server/json_util.hpp"
#include "http_server/http_util.hpp"

#include "db_service/problem_content_service.hpp"
#include "db_service/problem_core_service.hpp"
#include "db_service/problem_statistics_service.hpp"
#include "db_service/testcase_service.hpp"

problem_handler::response_type problem_handler::handle_list_problems_get(
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
        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to list problems: " + to_string(summary_values_exp.error()) + "\n"
        );
    }

    return json_util::create_json_response(
        request,
        boost::beast::http::status::ok,
        json_util::make_problem_list_object(*summary_values_exp)
    );
}

problem_handler::response_type problem_handler::handle_get_problem_get(
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
        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to check problem: " + to_string(exists_problem_exp.error()) + "\n"
        );
    }
    if(!exists_problem_exp->exists){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::not_found,
            "problem not found\n"
        );
    }

    const auto title_exp = problem_core_service::get_title(
        db_connection_value,
        problem_reference_value
    );
    if(!title_exp){
        return http_util::create_404_or_500_response(
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
        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to get problem version: " + to_string(version_exp.error()) + "\n"
        );
    }

    const auto limits_exp = problem_core_service::get_limits(
        db_connection_value,
        problem_reference_value
    );
    if(!limits_exp){
        return http_util::create_404_or_500_response(
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
        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to get problem statement: " + to_string(statement_exp.error()) + "\n"
        );
    }

    const auto samples_exp = problem_content_service::list_samples(
        db_connection_value,
        problem_reference_value
    );
    if(!samples_exp){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to list problem samples: " + to_string(samples_exp.error()) + "\n"
        );
    }

    const auto statistics_exp = problem_statistics_service::get_statistics(
        db_connection_value,
        problem_reference_value
    );
    if(!statistics_exp){
        return http_util::create_404_or_500_response(
            request,
            "get problem statistics",
            statistics_exp.error()
        );
    }

    return json_util::create_json_response(
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

problem_handler::response_type problem_handler::handle_create_problem_post(
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
            return http_util::create_text_response(
                request,
                boost::beast::http::status::internal_server_error,
                "failed to create problem: " + to_string(create_problem_exp.error()) + "\n"
            );
        }

        return json_util::create_json_response(
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

problem_handler::response_type problem_handler::handle_set_limits_put(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto limits_exp = http_util::parse_json_dto_or_400<problem_dto::limits>(
            request,
            problem_dto::make_limits_from_json
        );
        if(!limits_exp){
            return std::move(limits_exp.error());
        }

        const auto set_limits_exp = problem_core_service::set_limits(
            db_connection_value,
            problem_reference_value,
            *limits_exp
        );
        if(!set_limits_exp){
            return http_util::create_400_or_500_response(
                request,
                "set problem limits",
                set_limits_exp.error()
            );
        }

        return http_util::create_text_response(
            request,
            boost::beast::http::status::ok,
            "problem limits updated\n"
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

problem_handler::response_type problem_handler::handle_set_statement_put(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto statement_exp = http_util::parse_json_dto_or_400<problem_dto::statement>(
            request,
            problem_dto::make_statement_from_json
        );
        if(!statement_exp){
            return std::move(statement_exp.error());
        }

        const auto set_statement_exp = problem_content_service::set_statement(
            db_connection_value,
            problem_reference_value,
            *statement_exp
        );
        if(!set_statement_exp){
            return http_util::create_400_or_500_response(
                request,
                "set problem statement",
                set_statement_exp.error()
            );
        }

        return http_util::create_text_response(
            request,
            boost::beast::http::status::ok,
            "problem statement updated\n"
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

problem_handler::response_type problem_handler::handle_create_testcase_post(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto testcase_exp = http_util::parse_json_dto_or_400<problem_dto::testcase>(
            request,
            problem_dto::make_testcase_from_json
        );
        if(!testcase_exp){
            return std::move(testcase_exp.error());
        }

        const auto create_testcase_exp = testcase_service::create_testcase(
            db_connection_value,
            problem_reference_value,
            *testcase_exp
        );
        if(!create_testcase_exp){
            return http_util::create_400_or_500_response(
                request,
                "create testcase",
                create_testcase_exp.error()
            );
        }

        return json_util::create_json_response(
            request,
            boost::beast::http::status::created,
            json_util::make_problem_testcase_created_object(*create_testcase_exp)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}
