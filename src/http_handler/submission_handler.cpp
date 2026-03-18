#include "http_handler/submission_handler.hpp"
#include "dto/submission_dto.hpp"
#include "http_server/json_util.hpp"
#include "http_server/http_util.hpp"

#include "db/submission_core_service.hpp"
#include "db/submission_util.hpp"

submission_handler::response_type submission_handler::handle_create_submission_post(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t problem_id
){
    const auto auth_identity_exp = http_util::try_auth_bearer(request, db_connection_value);
    if(!auth_identity_exp){
        return std::move(auth_identity_exp.error());
    }

    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto source_opt = submission_dto::make_source(*request_object_opt);
    if(!source_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: language, source_code\n"
        );
    }

    const auto create_submission_exp = submission_core_service::create_submission(
        db_connection_value,
        auth_identity_exp->user_id,
        problem_id,
        *source_opt
    );
    if(!create_submission_exp){
        const auto code = create_submission_exp.error();
        const auto status =
            code.is_bad_request_error()
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to create submission: " + to_string(code) + "\n"
        );
    }

    submission_dto::created created_value;
    created_value.submission_id = create_submission_exp.value();
    created_value.status = to_string(submission_status::queued);

    return json_util::create_json_response(
        request,
        boost::beast::http::status::created,
        json_util::make_submission_created_object(created_value)
    );
}
