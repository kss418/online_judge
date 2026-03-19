#include "http_handler/problem_handler.hpp"
#include "dto/problem_dto.hpp"
#include "http_server/json_util.hpp"
#include "http_server/http_util.hpp"

#include "db_service/problem_content_service.hpp"
#include "db_service/problem_core_service.hpp"
#include "db_service/problem_statistics_service.hpp"
#include "db_service/testcase_service.hpp"

problem_handler::response_type problem_handler::handle_get_problem_get(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    const auto exists_problem_exp = problem_core_service::exists_problem(
        db_connection_value,
        problem_id
    );
    if(!exists_problem_exp){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to check problem: " + to_string(exists_problem_exp.error()) + "\n"
        );
    }
    if(!exists_problem_exp.value()){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::not_found,
            "problem not found\n"
        );
    }

    const auto version_exp = problem_core_service::get_version(db_connection_value, problem_id);
    if(!version_exp){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to get problem version: " + to_string(version_exp.error()) + "\n"
        );
    }

    const auto limits_exp = problem_core_service::get_limits(db_connection_value, problem_id);
    if(!limits_exp){
        const auto status =
            limits_exp.error().is_bad_request_error()
                ? boost::beast::http::status::not_found
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to get problem limits: " + to_string(limits_exp.error()) + "\n"
        );
    }

    std::optional<problem_dto::statement> statement_opt = std::nullopt;
    const auto statement_exp = problem_content_service::get_statement(
        db_connection_value,
        problem_id
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
        problem_id
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
        problem_id
    );
    if(!statistics_exp){
        const auto status =
            statistics_exp.error().is_bad_request_error()
                ? boost::beast::http::status::not_found
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to get problem statistics: " + to_string(statistics_exp.error()) + "\n"
        );
    }

    return json_util::create_json_response(
        request,
        boost::beast::http::status::ok,
        json_util::make_problem_detail_object(
            problem_id,
            version_exp.value(),
            limits_exp.value(),
            statement_opt,
            samples_exp.value(),
            statistics_exp.value()
        )
    );
}

problem_handler::response_type problem_handler::handle_create_problem_post(
    const request_type& request,
    db_connection& db_connection_value
){
    if(const auto auth_identity_exp = http_util::try_admin_auth_bearer(request, db_connection_value);
        !auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto create_problem_exp = problem_core_service::create_problem(db_connection_value);
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
        json_util::make_problem_created_object(create_problem_exp.value())
    );
}

problem_handler::response_type problem_handler::handle_set_limits_put(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    if(const auto auth_identity_exp = http_util::try_admin_auth_bearer(request, db_connection_value);
        !auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto limits_opt = problem_dto::make_limits(*request_object_opt);
    if(!limits_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: memory_limit_mb, time_limit_ms\n"
        );
    }

    const auto set_limits_exp = problem_core_service::set_limits(
        db_connection_value,
        problem_id,
        *limits_opt
    );
    if(!set_limits_exp){
        const auto code = set_limits_exp.error();
        const auto status =
            code.is_bad_request_error()
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to set problem limits: " + to_string(code) + "\n"
        );
    }

    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "problem limits updated\n"
    );
}

problem_handler::response_type problem_handler::handle_set_statement_put(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    if(const auto auth_identity_exp = http_util::try_admin_auth_bearer(request, db_connection_value);
        !auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto statement_opt = problem_dto::make_statement(*request_object_opt);
    if(!statement_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: description, input_format, output_format\n"
        );
    }

    const auto set_statement_exp = problem_content_service::set_statement(
        db_connection_value,
        problem_id,
        *statement_opt
    );
    if(!set_statement_exp){
        const auto code = set_statement_exp.error();
        const auto status =
            code.is_bad_request_error()
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to set problem statement: " + to_string(code) + "\n"
        );
    }

    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "problem statement updated\n"
    );
}

problem_handler::response_type problem_handler::handle_create_testcase_post(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    if(const auto auth_identity_exp = http_util::try_admin_auth_bearer(request, db_connection_value);
        !auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto testcase_opt = problem_dto::make_tc(*request_object_opt);
    if(!testcase_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: testcase_input, testcase_output\n"
        );
    }

    const auto create_testcase_exp = tc_service::create_tc(
        db_connection_value,
        problem_id,
        *testcase_opt
    );
    if(!create_testcase_exp){
        const auto code = create_testcase_exp.error();
        const auto status =
            code.is_bad_request_error()
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to create testcase: " + to_string(code) + "\n"
        );
    }

    return json_util::create_json_response(
        request,
        boost::beast::http::status::created,
        json_util::make_problem_testcase_created_object(
            create_testcase_exp->id,
            create_testcase_exp->order
        )
    );
}
