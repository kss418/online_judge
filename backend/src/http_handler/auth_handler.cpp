#include "http_handler/auth_handler.hpp"
#include "db_service/auth_service.hpp"
#include "db_service/login_service.hpp"
#include "dto/auth_dto.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_guard.hpp"
#include "request_parser/auth_request_parser.hpp"
#include "serializer/auth_json_serializer.hpp"

#include <string>

auth_handler::response_type auth_handler::post_sign_up(
    const request_type& request,
    db_connection& db_connection_value
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::sign_up_request& sign_up_request) -> response_type {
            const auto sign_up_exp = login_service::sign_up(
                db_connection_value,
                sign_up_request
            );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "sign up",
                std::move(sign_up_exp),
                auth_json_serializer::make_session_object,
                boost::beast::http::status::created
            );
        },
        request_guard::make_json_guard<auth_dto::sign_up_request>(
            auth_request_parser::parse_sign_up_request
        )
    );
}

auth_handler::response_type auth_handler::post_login(
    const request_type& request,
    db_connection& db_connection_value
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::credentials& credentials_value) -> response_type {
            const auto login_exp = login_service::login(
                db_connection_value,
                credentials_value
            );
            return http_response_util::create_json_or_4xx_or_500(
                request,
                "login",
                std::move(login_exp),
                auth_json_serializer::make_session_object,
                [&]{
                    return http_response_util::create_error(
                        request,
                        http_error{http_error_code::invalid_credentials}
                    );
                }
            );
        },
        request_guard::make_json_guard<auth_dto::credentials>(
            auth_request_parser::parse_credentials
        )
    );
}

auth_handler::response_type auth_handler::post_token_renew(
    const request_type& request,
    db_connection& db_connection_value
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::token& token_value) -> response_type {
            const auto renew_token_exp = auth_service::renew_token(
                db_connection_value,
                token_value
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "renew token",
                std::move(renew_token_exp),
                "token renewed",
                [&]{
                    return http_response_util::create_error(
                        request,
                        http_error{http_error_code::invalid_or_expired_token}
                    );
                }
            );
        },
        auth_guard::make_bearer_token_guard()
    );
}

auth_handler::response_type auth_handler::post_logout(
    const request_type& request,
    db_connection& db_connection_value
){
    return http_guard::run_or_respond(
        request,
        db_connection_value,
        [&](const auth_dto::token& token_value) -> response_type {
            const auto revoke_token_exp = auth_service::revoke_token(
                db_connection_value,
                token_value
            );
            return http_response_util::create_message_or_4xx_or_500(
                request,
                "logout",
                std::move(revoke_token_exp),
                "logged out",
                [&]{
                    return http_response_util::create_error(
                        request,
                        http_error{http_error_code::invalid_or_expired_token}
                    );
                }
            );
        },
        auth_guard::make_bearer_token_guard()
    );
}
