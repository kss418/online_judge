#include "http_handler/problem_command_handler.hpp"

#include "db_service/problem_core_service.hpp"
#include "db_service/submission_service.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/problem_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/problem_request_parser.hpp"
#include "serializer/problem_json_serializer.hpp"

problem_command_handler::response_type problem_command_handler::post_problem(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const auth_dto::identity&,
            const problem_dto::create_request& create_request) -> response_type {
            const auto create_problem_exp = problem_core_service::create_problem(
                context_value.db_connection_ref(),
                create_request
            );
            return http_adapter::json(
                context_value.request,
                std::move(create_problem_exp),
                problem_json_serializer::make_created_object,
                boost::beast::http::status::created
            );
        },
        auth_guard::make_admin_guard(),
        request_parse_guard::make_json_guard<problem_dto::create_request>(
            problem_request_parser::parse_create_request
        )
    );
}

problem_command_handler::response_type problem_command_handler::put_problem(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_reference_value](context_type& context_value,
            const auth_dto::identity&,
            const problem_dto::update_request& update_request) -> response_type {
            const auto update_problem_exp = problem_core_service::update_problem(
                context_value.db_connection_ref(),
                problem_reference_value,
                update_request
            );
            return http_adapter::message(
                context_value.request,
                std::move(update_problem_exp),
                "problem updated"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value),
        request_parse_guard::make_json_guard<problem_dto::update_request>(
            problem_request_parser::parse_update_request
        )
    );
}

problem_command_handler::response_type problem_command_handler::delete_problem(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_reference_value](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const auto delete_problem_exp = problem_core_service::delete_problem(
                context_value.db_connection_ref(),
                problem_reference_value
            );
            return http_adapter::message(
                context_value.request,
                std::move(delete_problem_exp),
                "problem deleted"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}

problem_command_handler::response_type problem_command_handler::post_problem_rejudge(
    context_type& context,
    std::int64_t problem_id
){
    problem_dto::reference problem_reference_value{problem_id};
    return http_guard::run_or_respond(
        context,
        [problem_id](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            const auto rejudge_problem_exp = submission_service::rejudge_problem(
                context_value.db_connection_ref(),
                problem_id
            );
            return http_adapter::message(
                context_value.request,
                std::move(rejudge_problem_exp),
                "problem submissions requeued"
            );
        },
        auth_guard::make_admin_guard(),
        problem_guard::make_exists_guard(problem_reference_value)
    );
}
