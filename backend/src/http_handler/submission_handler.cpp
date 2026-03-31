#include "http_handler/submission_handler.hpp"
#include "common/submission_status.hpp"
#include "db_service/submission_service.hpp"
#include "dto/submission_dto.hpp"
#include "http_core/http_util.hpp"
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
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto submission_history_exp = submission_service::get_submission_history(
            db_connection_value,
            submission_id
        );
        if(!submission_history_exp){
            return http_response_util::create_404_or_500(
                request,
                "get submission history",
                submission_history_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            submission_json_serializer::make_history_list_object(
                submission_id,
                *submission_history_exp
            )
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

submission_handler::response_type submission_handler::get_submission_source(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    const auto handle_authenticated =
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto submission_source_exp = submission_service::get_submission_source(
                db_connection_value,
                submission_id
            );
            if(!submission_source_exp){
                return http_response_util::create_404_or_500(
                    request,
                    "get submission source",
                    submission_source_exp.error()
                );
            }

            if(!http_util::is_owner_or_admin(
                auth_identity_value,
                submission_source_exp->user_id
            )){
                return http_response_util::create_error(
                    request,
                    boost::beast::http::status::forbidden,
                    "forbidden",
                    "submission source access denied"
                );
            }

            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                submission_json_serializer::make_source_object(*submission_source_exp)
            );
        };

    return http_util::with_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

submission_handler::response_type submission_handler::get_submission(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    const auto submission_detail_exp = submission_service::get_submission_detail(
        db_connection_value,
        submission_id
    );
    if(!submission_detail_exp){
        return http_response_util::create_404_or_500(
            request,
            "get submission detail",
            submission_detail_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        submission_json_serializer::make_detail_object(*submission_detail_exp)
    );
}

submission_handler::response_type submission_handler::post_submission_status_batch(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto batch_request_exp =
        http_util::parse_json_dto_or_400<submission_dto::status_batch_request>(
            request,
            submission_dto::make_status_batch_request_from_json
        );
    if(!batch_request_exp){
        return std::move(batch_request_exp.error());
    }

    const auto snapshot_values_exp = submission_service::get_submission_status_snapshots(
        db_connection_value,
        batch_request_exp->submission_ids
    );
    if(!snapshot_values_exp){
        return http_response_util::create_4xx_or_500(
            request,
            "get submission status batch",
            snapshot_values_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        submission_json_serializer::make_status_snapshot_batch_object(*snapshot_values_exp)
    );
}

submission_handler::response_type submission_handler::post_submission(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    const auto handle_authenticated =
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            const auto create_request_exp =
                http_util::parse_json_dto_or_400<submission_dto::create_request>(
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
            if(!create_submission_exp){
                if(is_submission_banned_error(create_submission_exp.error())){
                    return http_response_util::create_error(
                        request,
                        boost::beast::http::status::forbidden,
                        "submission_banned",
                        "submission is currently banned"
                    );
                }

                return http_response_util::create_4xx_or_500(
                    request,
                    "create submission",
                    create_submission_exp.error()
                );
            }

            return http_response_util::create_json(
                request,
                boost::beast::http::status::created,
                submission_json_serializer::make_created_object(*create_submission_exp)
            );
        };

    return http_util::with_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

submission_handler::response_type submission_handler::post_submission_rejudge(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t submission_id
){
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto rejudge_exp = submission_service::rejudge(
            db_connection_value,
            submission_id
        );
        if(!rejudge_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "rejudge submission",
                rejudge_exp.error()
            );
        }

        submission_dto::created created_value;
        created_value.submission_id = submission_id;
        created_value.status = to_string(submission_status::queued);
        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            submission_json_serializer::make_created_object(created_value)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

submission_handler::response_type submission_handler::get_submissions(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto auth_identity_opt_exp = http_util::try_optional_auth_bearer(
        request,
        db_connection_value
    );
    if(!auth_identity_opt_exp){
        return std::move(auth_identity_opt_exp.error());
    }

    const auto filter_exp = http_util::parse_submission_list_filter_or_400(request);
    if(!filter_exp){
        return std::move(filter_exp.error());
    }

    std::optional<std::int64_t> viewer_user_id_opt = std::nullopt;
    if(auth_identity_opt_exp->has_value()){
        viewer_user_id_opt = auth_identity_opt_exp->value().user_id;
    }

    const auto submission_summary_values_exp = submission_service::list_submissions(
        db_connection_value,
        *filter_exp,
        viewer_user_id_opt
    );
    if(!submission_summary_values_exp){
        return http_response_util::create_4xx_or_500(
            request,
            "list submissions",
            submission_summary_values_exp.error()
        );
    }

    const auto total_submission_count_exp = submission_service::count_submissions(
        db_connection_value,
        *filter_exp
    );
    if(!total_submission_count_exp){
        return http_response_util::create_4xx_or_500(
            request,
            "count submissions",
            total_submission_count_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        submission_json_serializer::make_list_object(
            *submission_summary_values_exp,
            *total_submission_count_exp
        )
    );
}
