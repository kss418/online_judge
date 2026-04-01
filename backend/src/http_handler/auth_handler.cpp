#include "http_handler/auth_handler.hpp"
#include "db_service/auth_service.hpp"
#include "db_service/login_service.hpp"
#include "dto/auth_dto.hpp"
#include "http_core/auth_guard.hpp"
#include "http_core/http_util.hpp"
#include "serializer/auth_json_serializer.hpp"

#include <string>

auth_handler::response_type auth_handler::post_sign_up(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto sign_up_request_exp =
        http_util::parse_json_dto_or_400<auth_dto::sign_up_request>(
        request,
        auth_dto::make_sign_up_request_from_json
    );
    if(!sign_up_request_exp){
        return std::move(sign_up_request_exp.error());
    }

    const auto sign_up_exp = login_service::sign_up(
        db_connection_value,
        *sign_up_request_exp
    );
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "sign up",
        std::move(sign_up_exp),
        auth_json_serializer::make_session_object,
        boost::beast::http::status::created
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
    return http_response_util::create_json_or_4xx_or_500(
        request,
        "login",
        std::move(login_exp),
        auth_json_serializer::make_session_object,
        [&]{
            return http_response_util::create_error(
                request,
                boost::beast::http::status::unauthorized,
                "invalid_credentials",
                "invalid credentials"
            );
        }
    );
}

auth_handler::response_type auth_handler::post_token_renew(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto token_exp = auth_guard::parse_bearer_token_or_401(request);
    if(!token_exp){
        return std::move(token_exp.error());
    }

    const auto renew_token_exp = auth_service::renew_token(
        db_connection_value,
        *token_exp
    );
    return http_response_util::create_message_or_4xx_or_500(
        request,
        "renew token",
        std::move(renew_token_exp),
        "token renewed",
        [&]{
            return http_response_util::create_bearer_error(
                request,
                "invalid_or_expired_token",
                "invalid, expired, or revoked token"
            );
        }
    );
}

auth_handler::response_type auth_handler::post_logout(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto token_exp = auth_guard::parse_bearer_token_or_401(request);
    if(!token_exp){
        return std::move(token_exp.error());
    }

    const auto revoke_token_exp = auth_service::revoke_token(
        db_connection_value,
        *token_exp
    );
    return http_response_util::create_message_or_4xx_or_500(
        request,
        "logout",
        std::move(revoke_token_exp),
        "logged out",
        [&]{
            return http_response_util::create_bearer_error(
                request,
                "invalid_or_expired_token",
                "invalid, expired, or revoked token"
            );
        }
    );
}
