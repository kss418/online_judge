#include "http_server/http_handler.hpp"
#include "http_server/http_util.hpp"

#include "db/auth_service.hpp"
#include "db/login_service.hpp"

#include <string>
#include <utility>

#include <boost/json.hpp>

std::expected<http_handler, error_code> http_handler::create(
    db_connection db_connection
){
    if(!db_connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return http_handler(std::move(db_connection));
}

http_handler::http_handler(db_connection db_connection) :
    db_connection_(std::move(db_connection)){}

http_handler::response_type http_handler::handle_health_get(const request_type& request){
    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "ok\n"
    );
}

http_handler::response_type http_handler::handle_sign_up_post(const request_type& request){
    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto& request_object = *request_object_opt;
    const auto user_login_id_opt = http_util::get_non_empty_string_field(
        request_object,
        "user_login_id"
    );

    const auto raw_password_opt = http_util::get_non_empty_string_field(
        request_object,
        "raw_password"
    );

    if(!user_login_id_opt || !raw_password_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: user_login_id, raw_password\n"
        );
    }

    const auto sign_up_exp = login_service::sign_up(
        db_connection_,
        *user_login_id_opt,
        *raw_password_opt
    );
    
    if(!sign_up_exp){
        const auto code = sign_up_exp.error();
        const bool is_invalid_argument_error = code == errno_error::invalid_argument;
        const bool is_constraint_error =
            code == psql_error::unique_violation ||
            code == psql_error::check_violation ||
            code == psql_error::not_null_violation;
        const auto status =
            (is_invalid_argument_error || is_constraint_error)
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to sign up: " + to_string(code) + "\n"
        );
    }

    boost::json::object response_object;
    response_object["user_id"] = sign_up_exp->user_id;
    response_object["is_admin"] = sign_up_exp->is_admin;
    response_object["token"] = sign_up_exp->token;

    auto response = http_util::create_text_response(
        request,
        boost::beast::http::status::created,
        boost::json::serialize(response_object) + "\n"
    );
    response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    return response;
}

http_handler::response_type http_handler::handle_login_post(const request_type& request){
    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto& request_object = *request_object_opt;
    const auto user_login_id_opt = http_util::get_non_empty_string_field(
        request_object,
        "user_login_id"
    );
    const auto raw_password_opt = http_util::get_non_empty_string_field(
        request_object,
        "raw_password"
    );

    if(!user_login_id_opt || !raw_password_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: user_login_id, raw_password\n"
        );
    }

    const auto login_exp = login_service::login(
        db_connection_,
        *user_login_id_opt,
        *raw_password_opt
    );
    if(!login_exp){
        const auto code = login_exp.error();
        const bool is_invalid_argument_error = code == errno_error::invalid_argument;
        const auto status = is_invalid_argument_error
            ? boost::beast::http::status::bad_request
            : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to login: " + to_string(code) + "\n"
        );
    }
    if(!login_exp->has_value()){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::unauthorized,
            "invalid credentials\n"
        );
    }

    boost::json::object response_object;
    response_object["user_id"] = login_exp->value().user_id;
    response_object["is_admin"] = login_exp->value().is_admin;
    response_object["token"] = login_exp->value().token;

    auto response = http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        boost::json::serialize(response_object) + "\n"
    );
    response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    return response;
}

http_handler::response_type http_handler::handle_token_renew_post(const request_type& request){
    const auto token_opt = http_util::get_bearer_token(request);
    if(!token_opt){
        return http_util::create_bearer_unauthorized_response(
            request,
            "missing or invalid bearer token\n"
        );
    }

    const auto renew_token_exp = auth_service::renew_token(db_connection_, *token_opt);
    if(!renew_token_exp){
        const auto code = renew_token_exp.error();
        const bool is_invalid_argument_error = code == errno_error::invalid_argument;
        if(is_invalid_argument_error){
            return http_util::create_bearer_unauthorized_response(
                request,
                "missing or invalid bearer token\n"
            );
        }

        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to renew token: " + to_string(code) + "\n"
        );
    }
    if(!renew_token_exp.value()){
        return http_util::create_bearer_unauthorized_response(
            request,
            "invalid, expired, or revoked token\n"
        );
    }

    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "token renewed\n"
    );
}

http_handler::response_type http_handler::handle_logout_post(const request_type& request){
    const auto token_opt = http_util::get_bearer_token(request);
    if(!token_opt){
        return http_util::create_bearer_unauthorized_response(
            request,
            "missing or invalid bearer token\n"
        );
    }

    const auto revoke_token_exp = auth_service::revoke_token(db_connection_, *token_opt);
    if(!revoke_token_exp){
        const auto code = revoke_token_exp.error();
        const bool is_invalid_argument_error = code == errno_error::invalid_argument;
        if(is_invalid_argument_error){
            return http_util::create_bearer_unauthorized_response(
                request,
                "missing or invalid bearer token\n"
            );
        }

        return http_util::create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to logout: " + to_string(code) + "\n"
        );
    }
    if(!revoke_token_exp.value()){
        return http_util::create_bearer_unauthorized_response(
            request,
            "invalid, expired, or revoked token\n"
        );
    }

    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "logged out\n"
    );
}
