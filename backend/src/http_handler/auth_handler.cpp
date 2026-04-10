#include "http_handler/auth_handler.hpp"
#include "db_service/auth_service.hpp"
#include "db_service/login_service.hpp"
#include "dto/auth_dto.hpp"
#include "error/auth_error.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/auth_request_parser.hpp"
#include "serializer/auth_json_serializer.hpp"

#include <string>

namespace{
    auth_handler::response_type create_login_error_response(
        const auth_handler::request_type& request,
        const service_error& error_value
    ){
        return http_adapter::error_or_4xx_or_500(
            request,
            error_value,
            auth_error::map_login_service_error
        );
    }

    auth_handler::response_type create_token_error_response(
        const auth_handler::request_type& request,
        const service_error& error_value
    ){
        return http_adapter::error_or_4xx_or_500(
            request,
            error_value,
            auth_error::map_token_service_error
        );
    }
}

auth_handler::response_type auth_handler::post_sign_up(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const auth_dto::sign_up_request& sign_up_request) -> response_type {
            return http_adapter::json(
                context_value.request,
                login_service::sign_up(
                    context_value.db_connection_ref(),
                    sign_up_request
                ),
                auth_json_serializer::make_session_object,
                boost::beast::http::status::created
            );
        },
        request_parse_guard::make_json_guard<auth_dto::sign_up_request>(
            auth_request_parser::parse_sign_up_request
        )
    );
}

auth_handler::response_type auth_handler::post_login(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const auth_dto::credentials& credentials_value) -> response_type {
            return http_adapter::json(
                context_value.request,
                login_service::login(
                    context_value.db_connection_ref(),
                    credentials_value
                ),
                create_login_error_response,
                auth_json_serializer::make_session_object
            );
        },
        request_parse_guard::make_json_guard<auth_dto::credentials>(
            auth_request_parser::parse_credentials
        )
    );
}

auth_handler::response_type auth_handler::post_token_renew(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const auth_dto::token& token_value) -> response_type {
            return http_adapter::message(
                context_value.request,
                auth_service::renew_token(
                    context_value.db_connection_ref(),
                    token_value
                ),
                create_token_error_response,
                "token renewed"
            );
        },
        auth_guard::make_bearer_token_guard()
    );
}

auth_handler::response_type auth_handler::post_logout(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const auth_dto::token& token_value) -> response_type {
            return http_adapter::message(
                context_value.request,
                auth_service::revoke_token(
                    context_value.db_connection_ref(),
                    token_value
                ),
                create_token_error_response,
                "logged out"
            );
        },
        auth_guard::make_bearer_token_guard()
    );
}
