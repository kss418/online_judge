#include "http_handler/problem_command_handler.hpp"

#include "application/problem_command_action.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

namespace{
    auto make_post_problem_guard(){
        return http_guard::make_composite_guard(
            [](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::create_request& create_request_value)
                -> std::expected<
                    create_problem_action::command,
                    problem_command_handler::response_type
                > {
                return create_request_value;
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_dto::create_request>(
                problem_request_parser::parse_create_request
            )
        );
    }

    auto make_post_problem_spec(){
        return http_endpoint::endpoint_spec{
            .parse = make_post_problem_guard(),
            .execute = [](problem_command_handler::context_type& context,
                const create_problem_action::command& command_value) {
                return create_problem_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = problem_json_serializer::make_created_object,
            .error_response = http_endpoint::default_error_response_factory{},
            .success_status = boost::beast::http::status::created
        };
    }

    auto make_put_problem_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::update_request& update_request_value)
                -> std::expected<
                    update_problem_action::command,
                    problem_command_handler::response_type
                > {
                return update_problem_action::command{
                    .problem_reference_value = problem_reference_value,
                    .update_request_value = update_request_value
                };
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value),
            request_parse_guard::make_json_guard<problem_dto::update_request>(
                problem_request_parser::parse_update_request
            )
        );
    }

    auto make_put_problem_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_put_problem_guard(problem_id),
            .execute = [](problem_command_handler::context_type& context,
                const update_problem_action::command& command_value) {
                return update_problem_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [](const problem_dto::mutation_result& mutation_value) {
                return problem_json_serializer::make_message_object(
                    "problem updated",
                    mutation_value
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_delete_problem_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    delete_problem_action::command,
                    problem_command_handler::response_type
                > {
                return problem_reference_value;
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_delete_problem_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_delete_problem_guard(problem_id),
            .execute = [](problem_command_handler::context_type& context,
                const delete_problem_action::command& command_value) {
                return delete_problem_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = []() -> std::string_view {
                return "problem deleted";
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_post_problem_rejudge_guard(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_guard::make_composite_guard(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    rejudge_problem_action::command,
                    problem_command_handler::response_type
                > {
                return rejudge_problem_action::command{
                    .problem_id = problem_id
                };
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_post_problem_rejudge_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = make_post_problem_rejudge_guard(problem_id),
            .execute = [](problem_command_handler::context_type& context,
                const rejudge_problem_action::command& command_value) {
                return rejudge_problem_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = []() -> std::string_view {
                return "problem submissions requeued";
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }
}

problem_command_handler::response_type problem_command_handler::post_problem(
    context_type& context
){
    return http_endpoint::run_json(context, make_post_problem_spec());
}

problem_command_handler::response_type problem_command_handler::put_problem(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_put_problem_spec(problem_id)
    );
}

problem_command_handler::response_type problem_command_handler::delete_problem(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_message(
        context,
        make_delete_problem_spec(problem_id)
    );
}

problem_command_handler::response_type problem_command_handler::post_problem_rejudge(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_message(
        context,
        make_post_problem_rejudge_spec(problem_id)
    );
}
