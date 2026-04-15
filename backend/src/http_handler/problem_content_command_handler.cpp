#include "http_handler/problem_content_command_handler.hpp"

#include "application/set_problem_limits_action.hpp"
#include "application/set_problem_statement_action.hpp"
#include "db_service/problem_content_service.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_endpoint/http_error_mapper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_content_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <string_view>

namespace{
    using response_type = problem_content_command_handler::response_type;

    struct create_problem_sample_request{
        problem_dto::reference problem_reference_value;
    };

    struct update_problem_sample_request{
        problem_content_dto::sample_ref sample_reference_value;
        problem_content_dto::sample sample_value;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_problem_message_serializer(std::string_view message){
        return [message](const problem_dto::mutation_result& mutation_value) {
            return problem_json_serializer::make_message_object(
                message,
                mutation_value
            );
        };
    }

    auto make_put_limits_spec(std::int64_t problem_id){
        return http_endpoint::make_guarded_json_spec(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_content_dto::limits& limits_value)
                -> command_expected<set_problem_limits_action::command> {
                return set_problem_limits_action::command{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .limits_value = limits_value
                };
            },
            http_endpoint::make_db_execute(set_problem_limits_action::execute),
            make_problem_message_serializer("problem limits updated"),
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_content_dto::limits>(
                problem_content_request_parser::parse_limits
            )
        );
    }

    auto make_put_statement_spec(std::int64_t problem_id){
        return http_endpoint::make_guarded_json_spec(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_content_dto::statement& statement_value)
                -> command_expected<set_problem_statement_action::command> {
                return set_problem_statement_action::command{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .statement_value = statement_value
                };
            },
            http_endpoint::make_db_execute(set_problem_statement_action::execute),
            make_problem_message_serializer("problem statement updated"),
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_content_dto::statement>(
                problem_content_request_parser::parse_statement
            )
        );
    }

    auto make_post_sample_spec(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};

        return http_endpoint::make_guarded_json_spec(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<create_problem_sample_request> {
                return create_problem_sample_request{
                    .problem_reference_value = problem_reference_value
                };
            },
            [](auto& context, const create_problem_sample_request& request) {
                return problem_content_service::create_sample(
                    context.db_connection_ref(),
                    request.problem_reference_value,
                    problem_content_dto::sample{}
                );
            },
            problem_json_serializer::make_versioned_sample_created_object,
            http_endpoint::spec_options{
                .success_status = boost::beast::http::status::created
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_put_sample_spec(std::int64_t problem_id, std::int32_t sample_order){
        problem_content_dto::sample_ref sample_reference_value{
            .problem_id = problem_id,
            .sample_order = sample_order
        };

        return http_endpoint::make_guarded_json_spec(
            [sample_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_content_dto::sample& sample_value)
                -> command_expected<update_problem_sample_request> {
                return update_problem_sample_request{
                    .sample_reference_value = sample_reference_value,
                    .sample_value = sample_value
                };
            },
            [](auto& context, const update_problem_sample_request& request) {
                return problem_content_service::set_sample_and_get(
                    context.db_connection_ref(),
                    request.sample_reference_value,
                    request.sample_value
                );
            },
            problem_json_serializer::make_versioned_sample_object,
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_content_dto::sample>(
                problem_content_request_parser::parse_sample
            )
        );
    }

    auto make_delete_sample_spec(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};

        return http_endpoint::make_guarded_json_spec(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<problem_dto::reference> {
                return problem_reference_value;
            },
            http_endpoint::make_db_execute(problem_content_service::delete_sample),
            make_problem_message_serializer("problem sample deleted"),
            http_endpoint::spec_options{
                .error_response = http_error_mapper::delete_sample_error()
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }
}

problem_content_command_handler::response_type problem_content_command_handler::put_limits(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(context, make_put_limits_spec(problem_id));
}

problem_content_command_handler::response_type problem_content_command_handler::put_statement(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(context, make_put_statement_spec(problem_id));
}

problem_content_command_handler::response_type problem_content_command_handler::post_sample(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(context, make_post_sample_spec(problem_id));
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
    return http_endpoint::run_json(context, make_delete_sample_spec(problem_id));
}
