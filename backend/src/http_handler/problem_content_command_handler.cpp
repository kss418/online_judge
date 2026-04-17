#include "http_handler/problem_content_command_handler.hpp"

#include "db_service/problem_content_service.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_endpoint/http_error_mapper.hpp"
#include "http_handler/admin_problem_spec_helper.hpp"
#include "http_handler/path_value_spec_helper.hpp"
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

    struct update_problem_limits_request{
        problem_dto::reference problem_reference_value;
        problem_content_dto::limits limits_value;
    };

    struct update_problem_statement_request{
        problem_dto::reference problem_reference_value;
        problem_content_dto::statement statement_value;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_put_limits_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&,
                const problem_content_dto::limits& limits_value)
                -> command_expected<update_problem_limits_request> {
                return update_problem_limits_request{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .limits_value = limits_value
                };
            },
            [](auto& connection, const update_problem_limits_request& request) {
                return problem_content_service::set_limits(
                    connection,
                    request.problem_reference_value,
                    request.limits_value
                );
            },
            http_handler_spec::make_json_message_serializer(
                "problem limits updated",
                problem_json_serializer::make_message_object
            ),
            request_parse_guard::make_json_guard<problem_content_dto::limits>(
                problem_content_request_parser::parse_limits
            )
        );
    }

    auto make_put_statement_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&,
                const problem_content_dto::statement& statement_value)
                -> command_expected<update_problem_statement_request> {
                return update_problem_statement_request{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .statement_value = statement_value
                };
            },
            [](auto& connection, const update_problem_statement_request& request) {
                return problem_content_service::set_statement(
                    connection,
                    request.problem_reference_value,
                    request.statement_value
                );
            },
            http_handler_spec::make_json_message_serializer(
                "problem statement updated",
                problem_json_serializer::make_message_object
            ),
            request_parse_guard::make_json_guard<problem_content_dto::statement>(
                problem_content_request_parser::parse_statement
            )
        );
    }

    auto make_post_sample_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&)
                -> command_expected<create_problem_sample_request> {
                return create_problem_sample_request{
                    .problem_reference_value = problem_dto::reference{problem_id}
                };
            },
            [](auto& db_connection, const create_problem_sample_request& request) {
                return problem_content_service::create_sample(
                    db_connection,
                    request.problem_reference_value,
                    problem_content_dto::sample{}
                );
            },
            problem_json_serializer::make_versioned_sample_created_object,
            http_endpoint::spec_options{
                .success_status = boost::beast::http::status::created
            }
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
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&)
                -> command_expected<problem_dto::reference> {
                return problem_dto::reference{problem_id};
            },
            problem_content_service::delete_sample,
            http_handler_spec::make_json_message_serializer(
                "problem sample deleted",
                problem_json_serializer::make_message_object
            ),
            http_endpoint::spec_options{
                .error_response = http_error_mapper::delete_sample_error()
            }
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
