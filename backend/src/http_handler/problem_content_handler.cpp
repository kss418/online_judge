#include "http_handler/problem_content_handler.hpp"

#include "application/set_problem_limits_action.hpp"
#include "application/set_problem_statement_action.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "error/problem_content_error.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_guard.hpp"

#include "db_service/problem_content_service.hpp"
#include "request_parser/problem_content_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <utility>

problem_content_handler::response_type problem_content_handler::get_limits(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [&context, &problem_reference_value]() -> response_type {
            const auto limits_exp = problem_content_service::get_limits(
                context.db_connection_ref(),
                problem_reference_value
            );
            return http_adapter::json(
                context.request,
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
    context_type& context,
    std::int64_t problem_id
){
    return http_guard::run_or_respond(
        context,
        [problem_id](context_type& context_value,
            const auth_dto::identity&,
            const problem_content_dto::limits& limits_value){
            set_problem_limits_action::command command_value{
                .problem_reference_value = problem_dto::reference{problem_id},
                .limits_value = limits_value
            };
            const auto set_limits_exp = set_problem_limits_action::execute(
                context_value.db_connection_ref(),
                command_value
            );
            return http_adapter::message(
                context_value.request,
                std::move(set_limits_exp),
                "problem limits updated"
            );
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<problem_content_dto::limits>(
            problem_content_request_parser::parse_limits
        )
    );
}

problem_content_handler::response_type problem_content_handler::put_statement(
    context_type& context,
    std::int64_t problem_id
){
    return http_guard::run_or_respond(
        context,
        [problem_id](context_type& context_value,
            const auth_dto::identity&,
            const problem_content_dto::statement& statement_value){
            set_problem_statement_action::command command_value{
                .problem_reference_value = problem_dto::reference{problem_id},
                .statement_value = statement_value
            };
            const auto set_statement_exp = set_problem_statement_action::execute(
                context_value.db_connection_ref(),
                command_value
            );
            return http_adapter::message(
                context_value.request,
                std::move(set_statement_exp),
                "problem statement updated"
            );
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<problem_content_dto::statement>(
            problem_content_request_parser::parse_statement
        )
    );
}

problem_content_handler::response_type problem_content_handler::get_samples(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [&context, &problem_reference_value]() -> response_type {
            const auto sample_values_exp = problem_content_service::list_samples(
                context.db_connection_ref(),
                problem_reference_value
            );
            return http_adapter::json(
                context.request,
                std::move(sample_values_exp),
                problem_json_serializer::make_sample_list_object
            );
        },
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

problem_content_handler::response_type problem_content_handler::post_sample(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_reference_value](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const problem_content_dto::sample sample_value{};
            const auto create_sample_exp = problem_content_service::create_sample(
                context_value.db_connection_ref(),
                problem_reference_value,
                sample_value
            );
            return http_adapter::json(
                context_value.request,
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
    context_type& context,
    std::int64_t problem_id,
    std::int32_t sample_order
){
    problem_content_dto::sample_ref sample_reference_value{
        .problem_id = problem_id,
        .sample_order = sample_order
    };
    return http_guard::run_or_respond(
        context,
        [sample_reference_value](context_type& context_value,
            const auth_dto::identity&,
            const problem_content_dto::sample& sample_value){
            const auto set_sample_exp = problem_content_service::set_sample(
                context_value.db_connection_ref(),
                sample_reference_value,
                sample_value
            );
            return http_adapter::response(
                context_value.request,
                std::move(set_sample_exp),
                [&context_value, &sample_reference_value]() -> response_type {
                    return http_adapter::json(
                        context_value.request,
                        problem_content_service::get_sample(
                            context_value.db_connection_ref(),
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
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_reference_value](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const auto sample_values_exp = problem_content_service::list_samples(
                context_value.db_connection_ref(),
                problem_reference_value
            );
            return http_adapter::response(
                context_value.request,
                std::move(sample_values_exp),
                [&](
                    const std::vector<problem_content_dto::sample>& sample_values
                ) -> response_type {
                    if(sample_values.empty()){
                        return http_adapter::error(
                            context_value.request,
                            problem_content_error::missing_sample_to_delete()
                        );
                    }

                    return http_adapter::message(
                        context_value.request,
                        problem_content_service::delete_sample(
                            context_value.db_connection_ref(),
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
