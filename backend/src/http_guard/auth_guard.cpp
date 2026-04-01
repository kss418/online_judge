#include "http_guard/auth_guard.hpp"

#include "common/permission_util.hpp"
#include "common/string_util.hpp"
#include "db_service/auth_service.hpp"

#include <string>
#include <utility>

#include <boost/beast/core/string.hpp>
#include <boost/beast/http/field.hpp>

std::expected<auth_dto::token, auth_guard::response_type> auth_guard::parse_bearer_token_or_401(
    const request_type& request
){
    const auto token_opt = get_bearer_token(request);
    if(!token_opt){
        return std::unexpected(http_response_util::create_bearer_error(
            request,
            "missing_or_invalid_bearer_token",
            "missing or invalid bearer token"
        ));
    }

    auth_dto::token token_value;
    token_value.value = std::string{*token_opt};
    return token_value;
}

std::expected<auth_dto::identity, auth_guard::response_type> auth_guard::require_auth(
    const request_type& request,
    db_connection& db_connection
){
    const auto token_exp = parse_bearer_token_or_401(request);
    if(!token_exp){
        return std::unexpected(std::move(token_exp.error()));
    }

    const auto auth_identity_exp = auth_service::auth_token(db_connection, *token_exp);
    if(!auth_identity_exp){
        const auto code = auth_identity_exp.error();
        if(code == errno_error::invalid_argument){
            return std::unexpected(http_response_util::create_bearer_error(
                request,
                "missing_or_invalid_bearer_token",
                "missing or invalid bearer token"
            ));
        }

        return std::unexpected(http_response_util::create_error(
            request,
            boost::beast::http::status::internal_server_error,
            "internal_server_error",
            "failed to authenticate token: " + to_string(code)
        ));
    }
    if(!auth_identity_exp->has_value()){
        return std::unexpected(http_response_util::create_bearer_error(
            request,
            "invalid_or_expired_token",
            "invalid, expired, or revoked token"
        ));
    }

    return auth_identity_exp->value();
}

std::expected<std::optional<auth_dto::identity>, auth_guard::response_type>
auth_guard::require_optional_auth(
    const request_type& request,
    db_connection& db_connection
){
    const auto authorization_field_it = request.find(boost::beast::http::field::authorization);
    if(authorization_field_it == request.end()){
        return std::optional<auth_dto::identity>{};
    }

    const auto auth_identity_exp = require_auth(request, db_connection);
    if(!auth_identity_exp){
        return std::unexpected(std::move(auth_identity_exp.error()));
    }

    return std::optional<auth_dto::identity>{*auth_identity_exp};
}

std::expected<auth_dto::identity, auth_guard::response_type> auth_guard::require_admin(
    const request_type& request,
    db_connection& db_connection
){
    const auto auth_identity_exp = require_auth(request, db_connection);
    if(!auth_identity_exp){
        return std::unexpected(std::move(auth_identity_exp.error()));
    }
    if(!permission_util::has_admin_access(auth_identity_exp->permission_level)){
        return std::unexpected(http_response_util::create_bearer_error(
            request,
            "admin_bearer_token_required",
            "admin bearer token required"
        ));
    }

    return *auth_identity_exp;
}

std::expected<auth_dto::identity, auth_guard::response_type>
auth_guard::require_superadmin(
    const request_type& request,
    db_connection& db_connection
){
    const auto auth_identity_exp = require_auth(request, db_connection);
    if(!auth_identity_exp){
        return std::unexpected(std::move(auth_identity_exp.error()));
    }
    if(!permission_util::has_superadmin_access(auth_identity_exp->permission_level)){
        return std::unexpected(http_response_util::create_bearer_error(
            request,
            "superadmin_bearer_token_required",
            "superadmin bearer token required"
        ));
    }

    return *auth_identity_exp;
}

std::optional<std::string_view> auth_guard::get_bearer_token(
    const request_type& request
){
    const auto authorization_field_it = request.find(boost::beast::http::field::authorization);
    if(authorization_field_it == request.end()){
        return std::nullopt;
    }

    std::string_view authorization_value{
        authorization_field_it->value().data(),
        authorization_field_it->value().size()
    };
    authorization_value = string_util::trim_left_whitespace(authorization_value);
    authorization_value = string_util::trim_right_whitespace(authorization_value);

    if(
        authorization_value.size() <= 7 ||
        !boost::beast::iequals(authorization_value.substr(0, 6), "Bearer") ||
        authorization_value[6] != ' '
    ){
        return std::nullopt;
    }

    const std::string_view token = string_util::trim_left_whitespace(
        authorization_value.substr(7)
    );
    if(token.empty()){
        return std::nullopt;
    }

    return token;
}
