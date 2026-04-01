#pragma once

#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"
#include "http_core/http_response_util.hpp"

#include <expected>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>

namespace auth_guard{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    std::optional<std::string_view> get_bearer_token(
        const request_type& request
    );
    std::expected<auth_dto::token, response_type> parse_bearer_token_or_401(
        const request_type& request
    );
    std::expected<auth_dto::identity, response_type> try_auth_bearer(
        const request_type& request,
        db_connection& db_connection
    );
    std::expected<std::optional<auth_dto::identity>, response_type> try_optional_auth_bearer(
        const request_type& request,
        db_connection& db_connection
    );
    std::expected<auth_dto::identity, response_type> try_admin_auth_bearer(
        const request_type& request,
        db_connection& db_connection
    );
    std::expected<auth_dto::identity, response_type> try_superadmin_auth_bearer(
        const request_type& request,
        db_connection& db_connection
    );

    template <typename callback_type>
    response_type with_auth_bearer(
        const request_type& request,
        db_connection& db_connection,
        callback_type&& callback
    ){
        const auto auth_identity_exp = try_auth_bearer(request, db_connection);
        if(!auth_identity_exp){
            return std::move(auth_identity_exp.error());
        }

        return std::invoke(
            std::forward<callback_type>(callback),
            *auth_identity_exp
        );
    }

    template <typename callback_type>
    response_type with_admin_auth_bearer(
        const request_type& request,
        db_connection& db_connection,
        callback_type&& callback
    ){
        const auto auth_identity_exp = try_admin_auth_bearer(request, db_connection);
        if(!auth_identity_exp){
            return std::move(auth_identity_exp.error());
        }

        return std::invoke(
            std::forward<callback_type>(callback),
            *auth_identity_exp
        );
    }

    template <typename callback_type>
    response_type with_superadmin_auth_bearer(
        const request_type& request,
        db_connection& db_connection,
        callback_type&& callback
    ){
        const auto auth_identity_exp = try_superadmin_auth_bearer(request, db_connection);
        if(!auth_identity_exp){
            return std::move(auth_identity_exp.error());
        }

        return std::invoke(
            std::forward<callback_type>(callback),
            *auth_identity_exp
        );
    }
}
