#include "http_handler/problem_content_handler.hpp"

#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/auth_guard.hpp"
#include "http_core/request_dto.hpp"
#include "http_core/http_util.hpp"

#include "db_service/problem_content_service.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <utility>

problem_content_handler::response_type problem_content_handler::get_limits(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_util::with_existing_problem(
        request,
        db_connection_value,
        problem_reference_value,
        [&]() -> response_type {
            const auto limits_exp = problem_content_service::get_limits(
                db_connection_value,
                problem_reference_value
            );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "get problem limits",
                std::move(limits_exp),
                [](const auto& limits_value){
                    boost::json::object response_object;
                    response_object["memory_limit_mb"] = limits_value.memory_mb;
                    response_object["time_limit_ms"] = limits_value.time_ms;
                    return response_object;
                }
            );
        }
    );
}

problem_content_handler::response_type problem_content_handler::put_limits(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto limits_exp =
            request_dto::parse_json_dto_or_400<problem_content_dto::limits>(
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
        return http_response_util::create_message_or_4xx_or_500(
            request,
            "set problem limits",
            std::move(set_limits_exp),
            "problem limits updated"
        );
    };

    return auth_guard::with_admin_auth_bearer(
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
            request_dto::parse_json_dto_or_400<problem_content_dto::statement>(
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
        return http_response_util::create_message_or_4xx_or_500(
            request,
            "set problem statement",
            std::move(set_statement_exp),
            "problem statement updated"
        );
    };

    return auth_guard::with_admin_auth_bearer(
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
    return http_util::with_existing_problem(
        request,
        db_connection_value,
        problem_reference_value,
        [&]() -> response_type {
            const auto sample_values_exp = problem_content_service::list_samples(
                db_connection_value,
                problem_reference_value
            );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "list problem samples",
                std::move(sample_values_exp),
                problem_json_serializer::make_sample_list_object
            );
        }
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
        return http_response_util::create_json_or_4xx_or_500(
            request,
            "create problem sample",
            std::move(create_sample_exp),
            problem_json_serializer::make_sample_created_object,
            boost::beast::http::status::created
        );
    };

    return auth_guard::with_admin_auth_bearer(
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
            request_dto::parse_json_dto_or_400<problem_content_dto::sample>(
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
        return http_response_util::create_response_or_4xx_or_500(
            request,
            "set problem sample",
            std::move(set_sample_exp),
            [&]() -> response_type {
                return http_response_util::create_json_or_4xx_or_500(
                    request,
                    "get problem sample",
                    problem_content_service::get_sample(
                        db_connection_value,
                        sample_reference_value
                    ),
                    problem_json_serializer::make_sample_object
                );
            }
        );
    };

    return auth_guard::with_admin_auth_bearer(
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
        return http_response_util::create_response_or_4xx_or_500(
            request,
            "list problem samples",
            std::move(sample_values_exp),
            [&](
                const std::vector<problem_content_dto::sample>& sample_values
            ) -> response_type {
                if(sample_values.empty()){
                    return http_response_util::create_error(
                        request,
                        boost::beast::http::status::bad_request,
                        "invalid_sample_delete_request",
                        "failed to delete sample: invalid argument"
                    );
                }

                return http_response_util::create_message_or_4xx_or_500(
                    request,
                    "delete problem sample",
                    problem_content_service::delete_sample(
                        db_connection_value,
                        problem_reference_value
                    ),
                    "problem sample deleted"
                );
            }
        );
    };

    return auth_guard::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}
