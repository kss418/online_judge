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
        return http_util::create_400_or_500_response(
            request,
            "sign up",
            sign_up_exp.error()
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
        return http_util::create_400_or_500_response(
            request,
            "login",
            login_exp.error()
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
