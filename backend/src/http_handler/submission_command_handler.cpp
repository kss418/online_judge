#include "http_handler/submission_command_handler.hpp"

#include "application/create_submission_action.hpp"
#include "application/submission_command_action.hpp"
#include "dto/submission_internal_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_endpoint/http_error_mapper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/submission_guard.hpp"
#include "serializer/submission_json_serializer.hpp"

namespace{
    auto make_post_submission_spec(std::int64_t problem_id){
        return http_endpoint::endpoint_spec{
            .parse = submission_guard::make_create_request_guard(problem_id),
            .execute = [](submission_command_handler::context_type& context,
                const submission_internal_dto::create_submission_command& command_value) {
                return create_submission_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = submission_json_serializer::make_queued_response_object,
            .error_response = http_error_mapper::create_submission_error(),
            .success_status = boost::beast::http::status::created
        };
    }

    auto make_post_submission_rejudge_guard(std::int64_t submission_id){
        return http_guard::make_composite_guard(
            [submission_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    rejudge_submission_action::command,
                    submission_command_handler::response_type
                > {
                return rejudge_submission_action::command{
                    .submission_id = submission_id
                };
            },
            auth_guard::make_admin_guard()
        );
    }

    auto make_post_submission_rejudge_spec(std::int64_t submission_id){
        return http_endpoint::endpoint_spec{
            .parse = make_post_submission_rejudge_guard(submission_id),
            .execute = [](submission_command_handler::context_type& context,
                const rejudge_submission_action::command& command_value) {
                return rejudge_submission_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = submission_json_serializer::make_queued_response_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }
}

submission_command_handler::response_type submission_command_handler::post_submission(
    context_type& context,
    std::int64_t problem_id
){
    return http_endpoint::run_json(
        context,
        make_post_submission_spec(problem_id)
    );
}

submission_command_handler::response_type submission_command_handler::post_submission_rejudge(
    context_type& context,
    std::int64_t submission_id
){
    return http_endpoint::run_json(
        context,
        make_post_submission_rejudge_spec(submission_id)
    );
}
