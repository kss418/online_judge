#include "http_handler/auth_handler.hpp"
#include "db_service/auth_service.hpp"
#include "db_service/login_service.hpp"
#include "dto/auth_dto.hpp"
#include "http_server/json_util.hpp"
#include "http_server/http_util.hpp"

#include <string>

auth_handler::response_type auth_handler::handle_sign_up_post(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto credentials_opt = auth_dto::make_credentials(*request_object_opt);
    if(!credentials_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: user_login_id, raw_password\n"
        );
    }

    const auto sign_up_exp = login_service::sign_up(
        db_connection_value,
        *credentials_opt
    );
    if(!sign_up_exp){
        const auto code = sign_up_exp.error();
        const auto status =
            code.is_bad_request_error()
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to sign up: " + to_string(code) + "\n"
        );
    }

    return json_util::create_json_response(
        request,
        boost::beast::http::status::created,
        json_util::make_auth_session_object(*sign_up_exp)
    );
}

auth_handler::response_type auth_handler::handle_login_post(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto request_object_opt = http_util::parse_json_object(request);
    if(!request_object_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto credentials_opt = auth_dto::make_credentials(*request_object_opt);
    if(!credentials_opt){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: user_login_id, raw_password\n"
        );
    }

    const auto login_exp = login_service::login(
        db_connection_value,
        *credentials_opt
    );
    if(!login_exp){
        const auto code = login_exp.error();
        const auto status = code.is_bad_request_error()
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

    return json_util::create_json_response(
        request,
        boost::beast::http::status::ok,
        json_util::make_auth_session_object(login_exp->value())
    );
}

auth_handler::response_type auth_handler::handle_token_renew_post(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto token_opt = http_util::get_bearer_token(request);
    if(!token_opt){
        return http_util::create_bearer_unauthorized_response(
            request,
            "missing or invalid bearer token\n"
        );
    }
    auth_dto::token token_value;
    token_value.value = std::string{*token_opt};

    const auto renew_token_exp = auth_service::renew_token(
        db_connection_value,
        token_value
    );
    if(!renew_token_exp){
        const auto code = renew_token_exp.error();
        if(code == errno_error::invalid_argument){
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

auth_handler::response_type auth_handler::handle_logout_post(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto token_opt = http_util::get_bearer_token(request);
    if(!token_opt){
        return http_util::create_bearer_unauthorized_response(
            request,
            "missing or invalid bearer token\n"
        );
    }
    auth_dto::token token_value;
    token_value.value = std::string{*token_opt};

    const auto revoke_token_exp = auth_service::revoke_token(
        db_connection_value,
        token_value
    );
    if(!revoke_token_exp){
        const auto code = revoke_token_exp.error();
        if(code == errno_error::invalid_argument){
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
