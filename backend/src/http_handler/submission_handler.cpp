#include "http_handler/submission_handler.hpp"
#include "application/create_submission_action.hpp"
#include "application/get_submission_detail_query.hpp"
#include "application/get_submission_history_query.hpp"
#include "application/get_submission_source_query.hpp"
#include "application/list_submissions_query.hpp"
#include "common/submission_status.hpp"
#include "db_service/submission_service.hpp"
#include "dto/submission_dto.hpp"
#include "error/submission_error.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_guard.hpp"
#include "request_parser/submission_request_parser.hpp"
#include "serializer/submission_json_serializer.hpp"

namespace{
    submission_handler::response_type create_submission_error_response(
        const submission_handler::request_type& request,
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

    submission_handler::response_type get_submission_source_error_response(
        const submission_handler::request_type& request,
        const get_submission_source_query::error& error_value
    ){
        return http_adapter::error_or_4xx_or_500(
            request,
            error_value,
            [](const get_submission_source_query::error& mapped_error_value)
                -> std::optional<http_error> {
                if(mapped_error_value.is_source_access_denied()){
                    return submission_error::source_access_denied();
                }

                return std::nullopt;
            },
            [](const get_submission_source_query::error& mapped_error_value)
                -> const service_error& {
                return mapped_error_value.service_error_value;
            }
        );
    }
}

submission_handler::response_type submission_handler::get_submission_history(
    context_type& context,
    std::int64_t submission_id
){
    return http_guard::run_or_respond(
        context,
        [submission_id](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            get_submission_history_query::command command_value{
                .submission_id = submission_id
            };
            return http_adapter::json(
                context_value.request,
                get_submission_history_query::execute(
                    context_value.db_connection_ref(),
                    command_value
                ),
                [submission_id](const submission_dto::history_list& history_values) {
                    return submission_json_serializer::make_history_list_object(
                        submission_id,
                        history_values
                    );
                }
            );
        },
        auth_guard::make_admin_guard()
    );
}

submission_handler::response_type submission_handler::get_submission_source(
    context_type& context,
    std::int64_t submission_id
){
    return http_guard::run_or_respond(
        context,
        [submission_id](context_type& context_value,
            const auth_dto::identity& auth_identity_value) -> response_type {
            get_submission_source_query::command command_value{
                .submission_id = submission_id,
                .viewer_identity = auth_identity_value
            };
            return http_adapter::json(
                context_value.request,
                get_submission_source_query::execute(
                    context_value.db_connection_ref(),
                    command_value
                ),
                get_submission_source_error_response,
                submission_json_serializer::make_source_object
            );
        },
        auth_guard::make_auth_guard()
    );
}

submission_handler::response_type submission_handler::get_submission(
    context_type& context,
    std::int64_t submission_id
){
    return http_guard::run_or_respond(
        context,
        [submission_id](context_type& context_value) -> response_type {
            get_submission_detail_query::command command_value{
                .submission_id = submission_id
            };
            return http_adapter::json(
                context_value.request,
                get_submission_detail_query::execute(
                    context_value.db_connection_ref(),
                    command_value
                ),
                submission_json_serializer::make_detail_object
            );
        }
    );
}

submission_handler::response_type submission_handler::post_submission_status_batch(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const submission_dto::status_batch_request& batch_request) -> response_type {
            return http_adapter::json(
                context_value.request,
                submission_service::get_submission_status_snapshots(
                    context_value.db_connection_ref(),
                    batch_request.submission_ids
                ),
                submission_json_serializer::make_status_snapshot_batch_object
            );
        },
        request_guard::make_json_guard<submission_dto::status_batch_request>(
            submission_request_parser::parse_status_batch_request
        )
    );
}

submission_handler::response_type submission_handler::post_submission(
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

submission_handler::response_type submission_handler::post_submission_rejudge(
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
                [&]{
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

submission_handler::response_type submission_handler::get_submissions(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const std::optional<auth_dto::identity>& auth_identity_opt,
            const submission_dto::list_filter& filter_value) -> response_type {
            list_submissions_query::command command_value{
                .filter_value = filter_value,
                .viewer_user_id_opt = auth_guard::get_viewer_user_id(auth_identity_opt)
            };
            return http_adapter::json(
                context_value.request,
                list_submissions_query::execute(
                    context_value.db_connection_ref(),
                    command_value
                ),
                submission_json_serializer::make_list_object
            );
        },
        auth_guard::make_optional_auth_guard(),
        request_guard::make_submission_list_filter_guard()
    );
}
