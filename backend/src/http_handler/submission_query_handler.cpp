#include "http_handler/submission_query_handler.hpp"

#include "application/get_submission_status_batch_query.hpp"
#include "application/get_submission_detail_query.hpp"
#include "application/get_submission_history_query.hpp"
#include "application/get_submission_source_query.hpp"
#include "application/list_submissions_query.hpp"
#include "dto/submission_request_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_endpoint/http_error_mapper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/submission_request_parser.hpp"
#include "serializer/submission_json_serializer.hpp"

namespace{
    auto make_get_submission_history_guard(std::int64_t submission_id){
        return http_guard::make_composite_guard(
            [submission_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    get_submission_history_query::command,
                    submission_query_handler::response_type
                > {
                return get_submission_history_query::command{
                    .submission_id = submission_id
                };
            },
            auth_guard::make_admin_guard()
        );
    }

    auto make_get_submission_history_spec(std::int64_t submission_id){
        return http_endpoint::endpoint_spec{
            .parse = make_get_submission_history_guard(submission_id),
            .execute = [](submission_query_handler::context_type& context,
                const get_submission_history_query::command& command_value) {
                return get_submission_history_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [submission_id](
                const submission_response_dto::history_list& history_values
            ) {
                return submission_json_serializer::make_history_list_object(
                    submission_id,
                    history_values
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_submission_source_guard(std::int64_t submission_id){
        return http_guard::make_composite_guard(
            [submission_id](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<
                    get_submission_source_query::command,
                    submission_query_handler::response_type
                > {
                return get_submission_source_query::command{
                    .submission_id = submission_id,
                    .viewer_identity = auth_identity_value
                };
            },
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_submission_source_spec(std::int64_t submission_id){
        return http_endpoint::endpoint_spec{
            .parse = make_get_submission_source_guard(submission_id),
            .execute = [](submission_query_handler::context_type& context,
                const get_submission_source_query::command& command_value) {
                return get_submission_source_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = submission_json_serializer::make_source_object,
            .error_response = http_error_mapper::submission_source_error()
        };
    }

    auto make_get_submission_spec(std::int64_t submission_id){
        return http_endpoint::endpoint_spec{
            .parse = http_endpoint::make_no_input_guard(),
            .execute = [submission_id](submission_query_handler::context_type& context,
                const http_endpoint::no_input&) {
                return get_submission_detail_query::execute(
                    context.db_connection_ref(),
                    get_submission_detail_query::command{
                        .submission_id = submission_id
                    }
                );
            },
            .serialize = submission_json_serializer::make_detail_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_post_submission_status_batch_spec(){
        return http_endpoint::endpoint_spec{
            .parse = request_parse_guard::make_json_guard<
                submission_request_dto::status_batch_request
            >(submission_request_parser::parse_status_batch_request),
            .execute = [](submission_query_handler::context_type& context,
                const get_submission_status_batch_query::command& command_value) {
                return get_submission_status_batch_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = submission_json_serializer::make_status_snapshot_batch_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_submissions_guard(){
        return http_guard::make_composite_guard(
            [](const http_guard::guard_context&,
                const std::optional<auth_dto::identity>& auth_identity_opt,
                const submission_request_dto::list_filter& filter_value)
                -> std::expected<
                    list_submissions_query::command,
                    submission_query_handler::response_type
                > {
                return list_submissions_query::command{
                    .filter_value = filter_value,
                    .viewer_user_id_opt = auth_guard::get_viewer_user_id(
                        auth_identity_opt
                    )
                };
            },
            auth_guard::make_optional_auth_guard(),
            request_parse_guard::make_submission_list_filter_guard()
        );
    }

    auto make_get_submissions_spec(){
        return http_endpoint::endpoint_spec{
            .parse = make_get_submissions_guard(),
            .execute = [](submission_query_handler::context_type& context,
                const list_submissions_query::command& command_value) {
                return list_submissions_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = submission_json_serializer::make_list_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }
}

submission_query_handler::response_type submission_query_handler::get_submission_history(
    context_type& context,
    std::int64_t submission_id
){
    return http_endpoint::run_json(
        context,
        make_get_submission_history_spec(submission_id)
    );
}

submission_query_handler::response_type submission_query_handler::get_submission_source(
    context_type& context,
    std::int64_t submission_id
){
    return http_endpoint::run_json(
        context,
        make_get_submission_source_spec(submission_id)
    );
}

submission_query_handler::response_type submission_query_handler::get_submission(
    context_type& context,
    std::int64_t submission_id
){
    return http_endpoint::run_json(
        context,
        make_get_submission_spec(submission_id)
    );
}

submission_query_handler::response_type submission_query_handler::post_submission_status_batch(
    context_type& context
){
    return http_endpoint::run_json(
        context,
        make_post_submission_status_batch_spec()
    );
}

submission_query_handler::response_type submission_query_handler::get_submissions(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_submissions_spec());
}
