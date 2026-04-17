#include "http_handler/submission_query_handler.hpp"

#include "application/get_submission_source_query.hpp"
#include "db_service/submission_query_service.hpp"
#include "dto/submission_request_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_endpoint/http_error_mapper.hpp"
#include "http_handler/optional_auth_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/submission_request_parser.hpp"
#include "serializer/submission_json_serializer.hpp"

namespace{
    using response_type = submission_query_handler::response_type;

    struct list_submissions_request{
        submission_request_dto::list_filter filter_value;
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_submission_history_spec(std::int64_t submission_id){
        return http_handler_spec::make_single_path_value_json_spec(
            submission_id,
            [](const http_guard::guard_context&,
                std::int64_t submission_id_value,
                const auth_dto::identity&)
                -> command_expected<std::int64_t> {
                return submission_id_value;
            },
            submission_query_service::get_submission_history,
            [submission_id](
                const submission_response_dto::history_list& history_values
            ) {
                return submission_json_serializer::make_history_list_object(
                    submission_id,
                    history_values
                );
            },
            auth_guard::make_admin_guard()
        );
    }

    auto make_get_submission_source_spec(std::int64_t submission_id){
        return http_handler_spec::make_single_path_value_json_spec(
            submission_id,
            [](const http_guard::guard_context&,
                std::int64_t submission_id_value,
                const auth_dto::identity& auth_identity_value)
                -> command_expected<get_submission_source_query::command> {
                return get_submission_source_query::command{
                    .submission_id = submission_id_value,
                    .viewer_identity = auth_identity_value
                };
            },
            get_submission_source_query::execute,
            submission_json_serializer::make_source_object,
            http_endpoint::spec_options{
                .error_response = http_error_mapper::submission_source_error()
            },
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_submission_spec(std::int64_t submission_id){
        return http_handler_spec::make_path_value_json_spec(
            submission_id,
            submission_query_service::get_submission_detail,
            submission_json_serializer::make_detail_object
        );
    }

    auto make_post_submission_status_batch_spec(){
        return http_endpoint::make_json_spec(
            request_parse_guard::make_json_guard<
                submission_request_dto::status_batch_request
            >(submission_request_parser::parse_status_batch_request),
            [](auto& context, const submission_request_dto::status_batch_request& request) {
                return submission_query_service::get_submission_status_snapshots(
                    context.db_connection_ref(),
                    request.submission_ids
                );
            },
            submission_json_serializer::make_status_snapshot_batch_object
        );
    }

    auto make_get_submissions_spec(){
        return http_handler_spec::make_auth_optional_json_spec(
            [](const http_guard::guard_context&,
                const std::optional<auth_dto::identity>& auth_identity_opt,
                const submission_request_dto::list_filter& filter_value)
                -> command_expected<list_submissions_request> {
                return list_submissions_request{
                    .filter_value = filter_value,
                    .viewer_user_id_opt = auth_guard::get_viewer_user_id(
                        auth_identity_opt
                    )
                };
            },
            [](auto& connection, const list_submissions_request& request) {
                return submission_query_service::list_submissions(
                    connection,
                    request.filter_value,
                    request.viewer_user_id_opt
                );
            },
            submission_json_serializer::make_list_object,
            request_parse_guard::make_submission_list_filter_guard()
        );
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
