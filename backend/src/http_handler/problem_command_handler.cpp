#include "http_handler/problem_command_handler.hpp"

#include "db_service/problem_command_service.hpp"
#include "db_service/submission_command_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/admin_problem_spec_helper.hpp"
#include "http_handler/path_value_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <string_view>

namespace{
    using response_type = problem_command_handler::response_type;

    struct update_problem_request{
        problem_dto::reference problem_reference_value;
        problem_dto::update_request update_request_value;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_post_problem_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::create_request& create_request_value)
                -> command_expected<problem_dto::create_request> {
                return create_request_value;
            },
            http_endpoint::make_db_execute(problem_command_service::create_problem),
            problem_json_serializer::make_created_object,
            http_endpoint::spec_options{
                .success_status = boost::beast::http::status::created
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<problem_dto::create_request>(
                problem_request_parser::parse_create_request
            )
        );
    }

    auto make_put_problem_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_json_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&,
                const problem_dto::update_request& update_request_value)
                -> command_expected<update_problem_request> {
                return update_problem_request{
                    .problem_reference_value = problem_dto::reference{problem_id},
                    .update_request_value = update_request_value
                };
            },
            [](auto& db_connection, const update_problem_request& request) {
                return problem_command_service::update_problem(
                    db_connection,
                    request.problem_reference_value,
                    request.update_request_value
                );
            },
            http_handler_spec::make_json_message_serializer(
                "problem updated",
                problem_json_serializer::make_message_object
            ),
            request_parse_guard::make_json_guard<problem_dto::update_request>(
                problem_request_parser::parse_update_request
            )
        );
    }

    auto make_delete_problem_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_message_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&)
                -> command_expected<problem_dto::reference> {
                return problem_dto::reference{problem_id};
            },
            problem_command_service::delete_problem,
            []() -> std::string_view {
                return "problem deleted";
            }
        );
    }

    auto make_post_problem_rejudge_spec(std::int64_t problem_id){
        return http_handler_spec::make_admin_problem_message_spec(
            problem_id,
            [](const http_guard::guard_context&,
                std::int64_t problem_id,
                const auth_dto::identity&)
                -> command_expected<std::int64_t> {
                return problem_id;
            },
            submission_command_service::rejudge_problem,
            []() -> std::string_view {
                return "problem submissions requeued";
            }
        );
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
