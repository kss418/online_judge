#include "http_handler/auth_handler.hpp"
#include "db_service/auth_service.hpp"
#include "db_service/login_service.hpp"
#include "dto/auth_dto.hpp"
#include "error/auth_error.hpp"
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
            if(!login_exp){
                if(login_exp.error() == service_error::unauthorized){
                    return http_response_util::create_error(
                        request,
                        auth_error::invalid_credentials()
                    );
                }

                return http_response_util::create_4xx_or_500(
                    request,
                    login_exp.error()
                );
            }

            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                auth_json_serializer::make_session_object(*login_exp)
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
            if(!renew_token_exp){
                if(renew_token_exp.error() == service_error::unauthorized){
                    return http_response_util::create_error(
                        request,
                        auth_error::invalid_or_expired_token()
                    );
                }

                return http_response_util::create_4xx_or_500(
                    request,
                    renew_token_exp.error()
                );
            }

            return http_response_util::create_message(
                request,
                boost::beast::http::status::ok,
                "token renewed"
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
            if(!revoke_token_exp){
                if(revoke_token_exp.error() == service_error::unauthorized){
                    return http_response_util::create_error(
                        request,
                        auth_error::invalid_or_expired_token()
                    );
                }

                return http_response_util::create_4xx_or_500(
                    request,
                    revoke_token_exp.error()
                );
            }

            return http_response_util::create_message(
                request,
                boost::beast::http::status::ok,
                "logged out"
            );
        },
        auth_guard::make_bearer_token_guard()
    );
}
