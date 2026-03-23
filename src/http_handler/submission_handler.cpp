#include "http_handler/submission_handler.hpp"
#include "common/submission_status.hpp"
#include "db_service/submission_service.hpp"
#include "dto/submission_dto.hpp"
#include "http_server/http_util.hpp"
#include "http_server/json_util.hpp"

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
                json_util::make_submission_source_object(*submission_source_exp)
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
        json_util::make_submission_detail_object(*submission_detail_exp)
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
                return http_response_util::create_400_or_500(
                    request,
                    "create submission",
                    create_submission_exp.error()
                );
            }

            return http_response_util::create_json(
                request,
                boost::beast::http::status::created,
                json_util::make_submission_created_object(*create_submission_exp)
            );
        };

    return http_util::with_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

submission_handler::response_type submission_handler::get_submissions(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto filter_exp = http_util::parse_submission_list_filter_or_400(request);
    if(!filter_exp){
        return std::move(filter_exp.error());
    }

    const auto submission_summary_values_exp = submission_service::list_submissions(
        db_connection_value,
        *filter_exp
    );
    if(!submission_summary_values_exp){
        return http_response_util::create_400_or_500(
            request,
            "list submissions",
            submission_summary_values_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_submission_list_object(*submission_summary_values_exp)
    );
}
