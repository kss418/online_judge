#include "http_handler/auth_command_handler.hpp"

#include "application/auth_command_action.hpp"
#include "dto/auth_dto.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_endpoint/http_error_mapper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/auth_request_parser.hpp"
#include "serializer/auth_json_serializer.hpp"

#include <string_view>

namespace{
    auto make_post_sign_up_spec(){
        return http_endpoint::make_json_spec(
            request_parse_guard::make_json_guard<auth_dto::sign_up_request>(
                auth_request_parser::parse_sign_up_request
            ),
            http_endpoint::make_db_execute(sign_up_action::execute),
            auth_json_serializer::make_session_object,
            http_endpoint::spec_options{
                .success_status = boost::beast::http::status::created
            }
        );
    }

    auto make_post_login_spec(){
        return http_endpoint::make_json_spec(
            request_parse_guard::make_json_guard<auth_dto::credentials>(
                auth_request_parser::parse_credentials
            ),
            http_endpoint::make_db_execute(login_action::execute),
            auth_json_serializer::make_session_object,
            http_endpoint::spec_options{
                .error_response = http_error_mapper::auth_login_error()
            }
        );
    }

    auto make_post_token_renew_spec(){
        return http_endpoint::make_message_spec(
            auth_guard::make_bearer_token_guard(),
            http_endpoint::make_db_execute(renew_auth_token_action::execute),
            []() -> std::string_view {
                return "token renewed";
            },
            http_endpoint::spec_options{
                .error_response = http_error_mapper::auth_token_error()
            }
        );
    }

    auto make_post_logout_spec(){
        return http_endpoint::make_message_spec(
            auth_guard::make_bearer_token_guard(),
            http_endpoint::make_db_execute(logout_action::execute),
            []() -> std::string_view {
                return "logged out";
            },
            http_endpoint::spec_options{
                .error_response = http_error_mapper::auth_token_error()
            }
        );
    }
}

auth_command_handler::response_type auth_command_handler::post_sign_up(
    context_type& context
){
    return http_endpoint::run_json(context, make_post_sign_up_spec());
}

auth_command_handler::response_type auth_command_handler::post_login(
    context_type& context
){
    return http_endpoint::run_json(context, make_post_login_spec());
}

auth_command_handler::response_type auth_command_handler::post_token_renew(
    context_type& context
){
    return http_endpoint::run_message(context, make_post_token_renew_spec());
}

auth_command_handler::response_type auth_command_handler::post_logout(
    context_type& context
){
    return http_endpoint::run_message(context, make_post_logout_spec());
}
