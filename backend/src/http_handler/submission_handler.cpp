#include "http_handler/submission_handler.hpp"
#include "common/submission_status.hpp"
#include "db_service/submission_service.hpp"
#include "dto/submission_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_guard.hpp"
#include "http_guard/submission_guard.hpp"
#include "request_parser/submission_request_parser.hpp"
#include "serializer/submission_json_serializer.hpp"

namespace{
    bool is_submission_banned_error(const service_error& code){
        return code == service_error::forbidden;
    }
}

submission_handler::response_type submission_handler::get_submission_history(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&,
            const submission_dto::history_list& history_values) -> response_type {
            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                submission_json_serializer::make_history_list_object(
                    submission_id,
                    history_values
                )
            );
        },
        auth_guard::make_admin_guard(),
        submission_guard::make_history_guard(submission_id)
    );
}

submission_handler::response_type submission_handler::get_submission_source(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const submission_dto::source_detail& source_detail_value) -> response_type {
            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                submission_json_serializer::make_source_object(source_detail_value)
            );
        },
        submission_guard::make_readable_source_guard(submission_id)
    );
}

submission_handler::response_type submission_handler::get_submission(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const submission_dto::detail& submission_detail_value) -> response_type {
            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                submission_json_serializer::make_detail_object(submission_detail_value)
            );
        },
        submission_guard::make_detail_guard(submission_id)
    );
}

submission_handler::response_type submission_handler::post_submission_status_batch(
    const request_type& request,
    db_connection& db_connection_value
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const submission_dto::status_batch_request& batch_request) -> response_type {
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "get submission status batch",
                submission_service::get_submission_status_snapshots(
                    db_connection_value,
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
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const submission_dto::create_request& create_request) -> response_type {
            const auto create_submission_exp = submission_service::create_submission(
                db_connection_value,
                create_request
            );
            return http_response_util::create_response_or_error(
                request,
                "create submission",
                std::move(create_submission_exp),
                [&](const request_type& error_request,
                    std::string_view action,
                    const service_error& code) {
                    if(is_submission_banned_error(code)){
                        return http_response_util::create_error(
                            error_request,
                            http_error{http_error_code::submission_banned}
                        );
                    }

                    return http_response_util::create_4xx_or_500(
                        error_request,
                        action,
                        code
                    );
                },
                [&](const submission_dto::created& created_value) {
                    return http_response_util::create_json(
                        request,
                        boost::beast::http::status::created,
                        submission_json_serializer::make_created_object(created_value)
                    );
                }
            );
        },
        request_guard::make_submission_create_request_guard(problem_id)
    );
}

submission_handler::response_type submission_handler::post_submission_rejudge(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::identity&) -> response_type {
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "rejudge submission",
                submission_service::rejudge(
                    db_connection_value,
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
    const request_type& request,
    db_connection& db_connection_value
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const std::optional<auth_dto::identity>& auth_identity_opt,
            const submission_dto::list_filter& filter_value) -> response_type {
            const auto viewer_user_id_opt = auth_guard::get_viewer_user_id(
                auth_identity_opt
            );

            return http_response_util::create_json_or_4xx_or_500(
                request,
                "list submissions",
                submission_service::list_submissions(
                    db_connection_value,
                    filter_value,
                    viewer_user_id_opt
                ),
                submission_json_serializer::make_list_object
            );
        },
        auth_guard::make_optional_auth_guard(),
        request_guard::make_submission_list_filter_guard()
    );
}
