#include "http_handler/auth_handler.hpp"
#include "db_service/auth_service.hpp"
#include "db_service/login_service.hpp"
#include "dto/auth_dto.hpp"
#include "http_server/json_util.hpp"
#include "http_server/http_util.hpp"

#include <string>

auth_handler::response_type auth_handler::post_sign_up(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto credentials_exp = http_util::parse_json_dto_or_400<auth_dto::credentials>(
        request,
        auth_dto::make_credentials_from_json
    );
    if(!credentials_exp){
        return std::move(credentials_exp.error());
    }

    const auto sign_up_exp = login_service::sign_up(
        db_connection_value,
        *credentials_exp
    );
    if(!sign_up_exp){
        return http_response_util::create_400_or_500(
            request,
            "sign up",
            sign_up_exp.error()
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::created,
        json_util::make_auth_session_object(*sign_up_exp)
    );
}

auth_handler::response_type auth_handler::post_login(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto credentials_exp = http_util::parse_json_dto_or_400<auth_dto::credentials>(
        request,
        auth_dto::make_credentials_from_json
    );
    if(!credentials_exp){
        return std::move(credentials_exp.error());
    }

    const auto login_exp = login_service::login(
        db_connection_value,
        *credentials_exp
    );
    if(!login_exp){
        return http_response_util::create_400_or_500(
            request,
            "login",
            login_exp.error()
        );
    }
    if(!login_exp->has_value()){
        return http_response_util::create_error(
            request,
            boost::beast::http::status::unauthorized,
            "invalid_credentials",
            "invalid credentials"
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_auth_session_object(login_exp->value())
    );
}

auth_handler::response_type auth_handler::post_token_renew(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto token_exp = http_util::parse_bearer_token_or_401(request);
    if(!token_exp){
        return std::move(token_exp.error());
    }

    const auto renew_token_exp = auth_service::renew_token(
        db_connection_value,
        *token_exp
    );
    if(!renew_token_exp){
        const auto code = renew_token_exp.error();
        if(code == errno_error::invalid_argument){
            return http_response_util::create_bearer_error(
                request,
                "missing_or_invalid_bearer_token",
                "missing or invalid bearer token"
            );
        }

        return http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to renew token: " + to_string(code)
        );
    }
    if(!renew_token_exp.value()){
        return http_response_util::create_bearer_error(
            request,
            "invalid_or_expired_token",
            "invalid, expired, or revoked token"
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_message_object("token renewed")
    );
}

auth_handler::response_type auth_handler::post_logout(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto token_exp = http_util::parse_bearer_token_or_401(request);
    if(!token_exp){
        return std::move(token_exp.error());
    }

    const auto revoke_token_exp = auth_service::revoke_token(
        db_connection_value,
        *token_exp
    );
    if(!revoke_token_exp){
        const auto code = revoke_token_exp.error();
        if(code == errno_error::invalid_argument){
            return http_response_util::create_bearer_error(
                request,
                "missing_or_invalid_bearer_token",
                "missing or invalid bearer token"
            );
        }

        return http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to logout: " + to_string(code)
        );
    }
    if(!revoke_token_exp.value()){
        return http_response_util::create_bearer_error(
            request,
            "invalid_or_expired_token",
            "invalid, expired, or revoked token"
        );
    }

    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_message_object("logged out")
    );
}
