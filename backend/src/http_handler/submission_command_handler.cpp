#include "http_handler/submission_command_handler.hpp"

#include "application/create_submission_action.hpp"
#include "db_service/submission_command_service.hpp"
#include "dto/submission_internal_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_endpoint/http_error_mapper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/submission_guard.hpp"
#include "serializer/submission_json_serializer.hpp"

namespace{
    using response_type = submission_command_handler::response_type;

    auto make_post_submission_spec(std::int64_t problem_id){
        return http_endpoint::make_json_spec(
            submission_guard::make_create_request_guard(problem_id),
            http_endpoint::make_db_execute(create_submission_action::execute),
            submission_json_serializer::make_queued_response_object,
            http_endpoint::spec_options{
                .error_response = http_error_mapper::create_submission_error(),
                .success_status = boost::beast::http::status::created
            }
        );
    }

    auto make_post_submission_rejudge_spec(std::int64_t submission_id){
        return http_endpoint::make_guarded_json_spec(
            [submission_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<std::int64_t, response_type> {
                return submission_id;
            },
            http_endpoint::make_db_execute(
                submission_command_service::rejudge_submission
            ),
            submission_json_serializer::make_queued_response_object,
            auth_guard::make_admin_guard()
        );
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
