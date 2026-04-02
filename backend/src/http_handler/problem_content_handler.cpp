#include "http_handler/problem_content_handler.hpp"

#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_guard.hpp"

#include "db_service/problem_content_service.hpp"
#include "request_parser/problem_content_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <utility>

problem_content_handler::response_type problem_content_handler::get_limits(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
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
        },
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

problem_content_handler::response_type problem_content_handler::put_limits(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&, const problem_content_dto::limits& limits_value){
            const auto set_limits_exp = problem_content_service::set_limits(
                db_connection_value,
                problem_reference_value,
                limits_value
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "set problem limits",
                std::move(set_limits_exp),
                "problem limits updated"
            );
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<problem_content_dto::limits>(
            problem_content_request_parser::parse_limits
        ),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

problem_content_handler::response_type problem_content_handler::put_statement(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&, const problem_content_dto::statement& statement_value){
            const auto set_statement_exp = problem_content_service::set_statement(
                db_connection_value,
                problem_reference_value,
                statement_value
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "set problem statement",
                std::move(set_statement_exp),
                "problem statement updated"
            );
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<problem_content_dto::statement>(
            problem_content_request_parser::parse_statement
        ),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

problem_content_handler::response_type problem_content_handler::get_samples(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
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
        },
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

problem_content_handler::response_type problem_content_handler::post_sample(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&) -> response_type {
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
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
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
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&, const problem_content_dto::sample& sample_value){
            const auto set_sample_exp = problem_content_service::set_sample(
                db_connection_value,
                sample_reference_value,
                sample_value
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
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<problem_content_dto::sample>(
            problem_content_request_parser::parse_sample
        )
    );
}

problem_content_handler::response_type problem_content_handler::delete_sample(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&) -> response_type {
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
                            http_error{
                                http_error_code::validation_error,
                                "missing sample to delete"
                            }
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
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}
