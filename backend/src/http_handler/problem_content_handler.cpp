#include "http_handler/problem_content_handler.hpp"

#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "http_server/json_util.hpp"
#include "http_server/http_util.hpp"

#include "db_service/problem_content_service.hpp"
#include "db_service/problem_core_service.hpp"

#include <utility>

problem_content_handler::response_type problem_content_handler::put_limits(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto limits_exp =
            http_util::parse_json_dto_or_400<problem_content_dto::limits>(
                request,
                problem_content_dto::make_limits_from_json
            );
        if(!limits_exp){
            return std::move(limits_exp.error());
        }

        const auto set_limits_exp = problem_content_service::set_limits(
            db_connection_value,
            problem_reference_value,
            *limits_exp
        );
        if(!set_limits_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "set problem limits",
                set_limits_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_message_object("problem limits updated")
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

problem_content_handler::response_type problem_content_handler::put_statement(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto statement_exp =
            http_util::parse_json_dto_or_400<problem_content_dto::statement>(
                request,
                problem_content_dto::make_statement_from_json
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
            return http_response_util::create_4xx_or_500(
                request,
                "set problem statement",
                set_statement_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_message_object("problem statement updated")
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

problem_content_handler::response_type problem_content_handler::get_samples(
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

    const auto sample_values_exp = problem_content_service::list_samples(
        db_connection_value,
        problem_reference_value
    );
    if(!sample_values_exp){
        return http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to list samples: " + to_string(sample_values_exp.error())
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_problem_sample_list_object(*sample_values_exp)
    );
}

problem_content_handler::response_type problem_content_handler::post_sample(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const problem_content_dto::sample sample_value{};

        const auto create_sample_exp = problem_content_service::create_sample(
            db_connection_value,
            problem_reference_value,
            sample_value
        );
        if(!create_sample_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "create problem sample",
                create_sample_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::created,
            json_util::make_problem_sample_created_object(*create_sample_exp)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

problem_content_handler::response_type problem_content_handler::put_sample(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id,
    std::int32_t sample_order
){
    problem_content_dto::sample_ref sample_reference_value{
        .problem_id = problem_id,
        .sample_order = sample_order
    };
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto sample_exp =
            http_util::parse_json_dto_or_400<problem_content_dto::sample>(
                request,
                problem_content_dto::make_sample_from_json
            );
        if(!sample_exp){
            return std::move(sample_exp.error());
        }

        const auto set_sample_exp = problem_content_service::set_sample(
            db_connection_value,
            sample_reference_value,
            *sample_exp
        );
        if(!set_sample_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "set problem sample",
                set_sample_exp.error()
            );
        }

        const auto updated_sample_exp = problem_content_service::get_sample(
            db_connection_value,
            sample_reference_value
        );
        if(!updated_sample_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "get problem sample",
                updated_sample_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_problem_sample_object(*updated_sample_exp)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

problem_content_handler::response_type problem_content_handler::delete_sample(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto sample_values_exp = problem_content_service::list_samples(
            db_connection_value,
            problem_reference_value
        );
        if(!sample_values_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "list problem samples",
                sample_values_exp.error()
            );
        }
        if(sample_values_exp->empty()){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                "invalid_sample_delete_request",
                "failed to delete sample: invalid argument"
            );
        }

        const auto delete_sample_exp = problem_content_service::delete_sample(
            db_connection_value,
            problem_reference_value
        );
        if(!delete_sample_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "delete problem sample",
                delete_sample_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_message_object("problem sample deleted")
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}
