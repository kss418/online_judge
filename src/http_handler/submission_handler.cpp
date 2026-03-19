#include "http_handler/submission_handler.hpp"
#include "common/submission_status.hpp"
#include "db_service/submission_service.hpp"
#include "dto/submission_dto.hpp"
#include "http_server/http_util.hpp"
#include "http_server/json_util.hpp"
#include "http_server/param_util.hpp"

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

    const auto create_request_opt = submission_dto::make_create_request_from_json(
        *request_object_opt,
        auth_identity_exp->user_id,
        problem_id
    );
    if(!create_request_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: language, source_code\n"
        );
    }

    const auto create_submission_exp = submission_service::create_submission(
        db_connection_value,
        *create_request_opt
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

    return json_util::create_json_response(
        request,
        boost::beast::http::status::created,
        json_util::make_submission_created_object(*create_submission_exp)
    );
}

submission_handler::response_type submission_handler::handle_list_submissions_get(
    const request_type& request,
    db_connection& db_connection_value
){
    const std::string_view target{
        request.target().data(),
        request.target().size()
    };
    const auto query_opt = http_util::get_target_query(target);
    const auto query_params_opt = http_util::parse_query_params(query_opt.value_or(""));
    if(!query_params_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid query string\n"
        );
    }

    submission_dto::list_filter filter_value;
    for(const auto& query_param : *query_params_opt){
        const auto error_message_opt = param_util::try_apply_submission_list_filter(
            query_param.key,
            query_param.value,
            filter_value
        );
        if(error_message_opt){
            return http_util::create_text_response(
                request,
                boost::beast::http::status::bad_request,
                *error_message_opt + "\n"
            );
        }
    }

    const auto submission_summary_values_exp = submission_service::list_submissions(
        db_connection_value,
        filter_value
    );
    if(!submission_summary_values_exp){
        const auto code = submission_summary_values_exp.error();
        const auto status =
            code.is_bad_request_error()
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to list submissions: " + to_string(code) + "\n"
        );
    }

    return json_util::create_json_response(
        request,
        boost::beast::http::status::ok,
        json_util::make_submission_list_object(*submission_summary_values_exp)
    );
}
