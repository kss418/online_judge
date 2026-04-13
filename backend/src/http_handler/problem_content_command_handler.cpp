#include "http_handler/problem_content_command_handler.hpp"

#include "application/problem_sample_action.hpp"
#include "application/set_problem_limits_action.hpp"
#include "application/set_problem_statement_action.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_endpoint/http_error_mapper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_content_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    auto make_put_limits_guard(std::int64_t problem_id){
        return http_guard::make_composite_guard(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_content_dto::limits& limits_value)
                -> std::expected<
                    set_problem_limits_action::command,
                    problem_content_command_handler::response_type
                > {
                return set_problem_limits_action::command{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .limits_value = limits_value
                };
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_content_dto::limits>(
                problem_content_request_parser::parse_limits
            )
        );
    }

    auto make_put_limits_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_put_limits_guard(problem_id),
            .execute = [](problem_content_command_handler::context_type& context,
                const set_problem_limits_action::command& command_value) {
                return set_problem_limits_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [](const problem_dto::mutation_result& mutation_value) {
                return problem_json_serializer::make_message_object(
                    "problem limits updated",
                    mutation_value
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_put_statement_guard(std::int64_t problem_id){
        return http_guard::make_composite_guard(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_content_dto::statement& statement_value)
                -> std::expected<
                    set_problem_statement_action::command,
                    problem_content_command_handler::response_type
                > {
                return set_problem_statement_action::command{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .statement_value = statement_value
                };
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_content_dto::statement>(
                problem_content_request_parser::parse_statement
            )
        );
    }

    auto make_put_statement_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_put_statement_guard(problem_id),
            .execute = [](problem_content_command_handler::context_type& context,
                const set_problem_statement_action::command& command_value) {
                return set_problem_statement_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [](const problem_dto::mutation_result& mutation_value) {
                return problem_json_serializer::make_message_object(
                    "problem statement updated",
                    mutation_value
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_post_sample_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    create_problem_sample_action::command,
                    problem_content_command_handler::response_type
                > {
                return create_problem_sample_action::command{
                    .problem_reference_value = problem_reference_value
                };
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_post_sample_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_post_sample_guard(problem_id),
            .execute = [](problem_content_command_handler::context_type& context,
                const create_problem_sample_action::command& command_value) {
                return create_problem_sample_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = problem_json_serializer::make_versioned_sample_created_object,
            .error_response = http_endpoint::default_error_response_factory{},
            .success_status = boost::beast::http::status::created
        };
    }

    auto make_put_sample_guard(std::int64_t problem_id, std::int32_t sample_order){
        problem_content_dto::sample_ref sample_reference_value{
            .problem_id = problem_id,
            .sample_order = sample_order
        };
        return http_guard::make_composite_guard(
            [sample_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_content_dto::sample& sample_value)
                -> std::expected<
                    update_problem_sample_action::command,
                    problem_content_command_handler::response_type
                > {
                return update_problem_sample_action::command{
                    .sample_reference_value = sample_reference_value,
                    .sample_value = sample_value
                };
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_content_dto::sample>(
                problem_content_request_parser::parse_sample
            )
        );
    }

    auto make_put_sample_spec(std::int64_t problem_id, std::int32_t sample_order){
        return http_endpoint::endpoint_spec{
            .parse = make_put_sample_guard(problem_id, sample_order),
            .execute = [](problem_content_command_handler::context_type& context,
                const update_problem_sample_action::command& command_value) {
                return update_problem_sample_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = problem_json_serializer::make_versioned_sample_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_delete_sample_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    delete_problem_sample_action::command,
                    problem_content_command_handler::response_type
                > {
                return problem_reference_value;
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_delete_sample_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_delete_sample_guard(problem_id),
            .execute = [](problem_content_command_handler::context_type& context,
                const delete_problem_sample_action::command& command_value) {
                return delete_problem_sample_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [](const problem_dto::mutation_result& mutation_value) {
                return problem_json_serializer::make_message_object(
                    "problem sample deleted",
                    mutation_value
                );
            },
            .error_response = http_error_mapper::delete_sample_error()
        };
    }
}

problem_content_command_handler::response_type problem_content_command_handler::put_limits(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_put_limits_spec(problem_id)
    );
}

problem_content_command_handler::response_type problem_content_command_handler::put_statement(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_put_statement_spec(problem_id)
    );
}

problem_content_command_handler::response_type problem_content_command_handler::post_sample(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_post_sample_spec(problem_id)
    );
}

problem_content_command_handler::response_type problem_content_command_handler::put_sample(
    context_type& context,
    std::int64_t problem_id,
    std::int32_t sample_order
){
    return http_endpoint::run_json(
        context,
        make_put_sample_spec(problem_id, sample_order)
    );
}

problem_content_command_handler::response_type problem_content_command_handler::delete_sample(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_delete_sample_spec(problem_id)
    );
}
