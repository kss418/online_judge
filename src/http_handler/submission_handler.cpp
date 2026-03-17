#include "http_handler/submission_handler.hpp"
#include "http_server/http_util.hpp"

#include "db/auth_service.hpp"
#include "db/submission_core_service.hpp"
#include "db/submission_util.hpp"

#include <boost/json.hpp>

submission_handler::submission_handler(db_connection& db_connection) :
    db_connection_(db_connection){}

bool submission_handler::is_submission_path(std::string_view path){
    return path.starts_with(path_prefix_);
}

submission_handler::response_type submission_handler::handle_create_submission_post(
    const request_type& request
){
    const auto token_opt = http_util::get_bearer_token(request);
    if(!token_opt){
        return http_util::create_bearer_unauthorized_response(
            request,
            "missing or invalid bearer token\n"
        );
    }

    const auto auth_identity_exp = auth_service::auth_token(db_connection_, *token_opt);
    if(!auth_identity_exp){
        const auto code = auth_identity_exp.error();
        if(code == errno_error::invalid_argument){
            return http_util::create_bearer_unauthorized_response(
                request,
                "missing or invalid bearer token\n"
            );
        }

        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to authenticate token: " + to_string(code) + "\n"
        );
    }
    if(!auth_identity_exp->has_value()){
        return http_util::create_bearer_unauthorized_response(
            request,
            "invalid, expired, or revoked token\n"
        );
    }

    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto& request_object = *request_object_opt;
    const auto problem_id_opt = http_util::get_positive_int64_field(request_object, "problem_id");
    const auto language_opt = http_util::get_non_empty_string_field(request_object, "language");
    const auto source_code_opt = http_util::get_non_empty_string_field(request_object, "source_code");

    if(!problem_id_opt || !language_opt || !source_code_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: problem_id, language, source_code\n"
        );
    }

    const auto create_submission_exp = submission_core_service::create_submission(
        db_connection_,
        auth_identity_exp->value().user_id,
        *problem_id_opt,
        *language_opt,
        *source_code_opt
    );
    if(!create_submission_exp){
        const auto code = create_submission_exp.error();
        const bool is_invalid_argument_error = code == errno_error::invalid_argument;
        const bool is_constraint_error =
            code == psql_error::foreign_key_violation ||
            code == psql_error::not_null_violation ||
            code == psql_error::check_violation;
        const auto status =
            (is_invalid_argument_error || is_constraint_error)
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to create submission: " + to_string(code) + "\n"
        );
    }

    boost::json::object response_object;
    response_object["submission_id"] = create_submission_exp.value();
    response_object["status"] = to_string(submission_status::queued);

    auto response = http_util::create_text_response(
        request,
        boost::beast::http::status::created,
        boost::json::serialize(response_object) + "\n"
    );
    response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    return response;
}
