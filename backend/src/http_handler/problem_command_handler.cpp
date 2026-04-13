#include "http_handler/problem_command_handler.hpp"

#include "application/problem_command_action.hpp"
#include "dto/problem_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

#include <string_view>

namespace{
    using response_type = problem_command_handler::response_type;

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

    auto make_post_problem_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::create_request& create_request_value)
                -> command_expected<create_problem_action::command> {
                return create_request_value;
            },
            http_endpoint::make_db_execute(create_problem_action::execute),
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
        problem_dto::reference problem_reference_value{problem_id};
        return http_endpoint::make_guarded_json_spec(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&,
                const problem_dto::update_request& update_request_value)
                -> command_expected<update_problem_action::command> {
                return update_problem_action::command{
                    .problem_reference_value = problem_reference_value,
                    .update_request_value = update_request_value
                };
            },
            http_endpoint::make_db_execute(update_problem_action::execute),
            make_problem_message_serializer("problem updated"),
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value),
            request_parse_guard::make_json_guard<problem_dto::update_request>(
                problem_request_parser::parse_update_request
            )
        );
    }

    auto make_delete_problem_spec(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_endpoint::make_guarded_message_spec(
            [problem_reference_value](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<delete_problem_action::command> {
                return problem_reference_value;
            },
            http_endpoint::make_db_execute(delete_problem_action::execute),
            []() -> std::string_view {
                return "problem deleted";
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
        );
    }

    auto make_post_problem_rejudge_spec(std::int64_t problem_id){
        problem_dto::reference problem_reference_value{problem_id};
        return http_endpoint::make_guarded_message_spec(
            [problem_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<rejudge_problem_action::command> {
                return rejudge_problem_action::command{
                    .problem_id = problem_id
                };
            },
            http_endpoint::make_db_execute(rejudge_problem_action::execute),
            []() -> std::string_view {
                return "problem submissions requeued";
            },
            auth_guard::make_admin_guard(),
            problem_guard::make_exists_guard(problem_reference_value)
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
