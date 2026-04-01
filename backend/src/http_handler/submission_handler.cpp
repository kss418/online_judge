#include "http_handler/submission_handler.hpp"
#include "common/submission_status.hpp"
#include "db_service/submission_service.hpp"
#include "dto/submission_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/submission_guard.hpp"
#include "http_core/request_dto.hpp"
#include "serializer/submission_json_serializer.hpp"

namespace{
    bool is_submission_banned_error(const error_code& code){
        return code == errno_error::permission_denied;
    }
}

submission_handler::response_type submission_handler::get_submission_history(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
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
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const auth_dto::identity& auth_identity_value,
            const submission_dto::source_detail& source_detail_value) -> response_type {
            const auto source_access_exp = submission_guard::require_source_access(
                request,
                auth_identity_value,
                source_detail_value
            );
            if(!source_access_exp){
                return std::move(source_access_exp.error());
            }

            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                submission_json_serializer::make_source_object(source_detail_value)
            );
        },
        auth_guard::make_auth_guard(),
        submission_guard::make_source_detail_guard(submission_id)
    );
}

submission_handler::response_type submission_handler::get_submission(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
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
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
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
        request_dto::make_json_guard<submission_dto::status_batch_request>(
            submission_dto::make_status_batch_request_from_json
        )
    );
}

submission_handler::response_type submission_handler::post_submission(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto create_request_exp =
                request_dto::parse_json_or_400<submission_dto::create_request>(
                    request,
                    submission_dto::make_create_request_from_json,
                    auth_identity_value.user_id,
                    problem_id
                );
            if(!create_request_exp){
                return std::move(create_request_exp.error());
            }

            const auto create_submission_exp = submission_service::create_submission(
                db_connection_value,
                *create_request_exp
            );
            return http_response_util::create_response_or_error(
                request,
                "create submission",
                std::move(create_submission_exp),
                [&](const request_type& error_request, std::string_view action, const error_code& code) {
                    if(is_submission_banned_error(code)){
                        return http_response_util::create_error(
                            error_request,
                            boost::beast::http::status::forbidden,
                            "submission_banned",
                            "submission is currently banned"
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
        auth_guard::make_auth_guard()
    );
}

submission_handler::response_type submission_handler::post_submission_rejudge(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
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
    const http_guard::guard_context guard_context{
        .request = request,
        .db_connection_value = db_connection_value
    };
    return http_guard::run_or_respond(
        guard_context,
        [&](const std::optional<auth_dto::identity>& auth_identity_opt,
            const submission_dto::list_filter& filter_value) -> response_type {
            std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
            if(auth_identity_opt.has_value()){
                viewer_user_id_opt = auth_identity_opt->user_id;
            }

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
        request_dto::make_submission_list_filter_guard()
    );
}
