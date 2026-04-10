#include "http_handler/submission_command_handler.hpp"

#include "application/create_submission_action.hpp"
#include "common/submission_status.hpp"
#include "db_service/submission_service.hpp"
#include "dto/submission_dto.hpp"
#include "error/submission_error.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_guard.hpp"
#include "serializer/submission_json_serializer.hpp"

namespace{
    submission_command_handler::response_type create_submission_error_response(
        const submission_command_handler::request_type& request,
        const create_submission_action::error& error_value
    ){
        return http_adapter::error_or_4xx_or_500(
            request,
            error_value,
            [](const create_submission_action::error& mapped_error_value)
                -> std::optional<http_error> {
                if(mapped_error_value.is_submission_banned()){
                    return submission_error::submission_banned();
                }

                return std::nullopt;
            },
            [](const create_submission_action::error& mapped_error_value)
                -> const service_error& {
                return mapped_error_value.service_error_value;
            }
        );
    }
}

submission_command_handler::response_type submission_command_handler::post_submission(
    context_type& context,
    std::int64_t problem_id
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const submission_dto::create_request& create_request) -> response_type {
            const auto create_submission_exp = create_submission_action::execute(
                context_value.db_connection_ref(),
                create_request
            );
            return http_adapter::json(
                context_value.request,
                std::move(create_submission_exp),
                create_submission_error_response,
                submission_json_serializer::make_created_object,
                boost::beast::http::status::created
            );
        },
        request_guard::make_submission_create_request_guard(problem_id)
    );
}

submission_command_handler::response_type submission_command_handler::post_submission_rejudge(
    context_type& context,
    std::int64_t submission_id
){
    return http_guard::run_or_respond(
        context,
        [submission_id](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            return http_adapter::json(
                context_value.request,
                submission_service::rejudge(
                    context_value.db_connection_ref(),
                    submission_id
                ),
                [submission_id]{
                    submission_dto::created created_value;
                    created_value.submission_id = submission_id;
                    created_value.status = to_string(submission_status::queued);
                    return submission_json_serializer::make_created_object(created_value);
                }
            );
        },
        auth_guard::make_admin_guard()
    );
}
