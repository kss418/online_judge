#pragma once

#include "http_server/http_response_util.hpp"

#include "db_service/auth_service.hpp"
#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"
#include "dto/dto_validation_error.hpp"
#include "dto/problem_dto.hpp"
#include "dto/submission_dto.hpp"

#include <boost/json.hpp>

#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace http_util{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    struct query_param{
        std::string_view key;
        std::string_view value;
    };

    std::optional<boost::json::object> parse_json_object(
        const request_type& request
    );
    template <typename dto_type, typename factory_type, typename... arg_types>
    std::expected<dto_type, response_type> parse_json_dto_or_400(
        const request_type& request,
        factory_type&& factory,
        arg_types&&... args
    ){
        const auto request_object_opt = parse_json_object(request);
        if(!request_object_opt){
            return std::unexpected(http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                "invalid_json",
                "invalid json"
            ));
        }

        auto dto_exp = std::invoke(
            std::forward<factory_type>(factory),
            *request_object_opt,
            std::forward<arg_types>(args)...
        );
        if(!dto_exp){
            const auto& validation_error = dto_exp.error();
            return std::unexpected(http_response_util::create_error(
                request,
                boost::beast::http::status::bad_request,
                validation_error.code,
                validation_error.message,
                validation_error.field_opt
            ));
        }

        return std::move(*dto_exp);
    }
    std::optional<std::string_view> get_string_field(
        const boost::json::object& object,
        std::string_view key
    );
    std::optional<std::string_view> get_non_empty_string_field(
        const boost::json::object& object,
        std::string_view key
    );
    std::optional<std::int64_t> get_positive_int64_field(
        const boost::json::object& object,
        std::string_view key
    );
    std::optional<std::int32_t> get_positive_int32_field(
        const boost::json::object& object,
        std::string_view key
    );
    std::optional<std::int32_t> get_non_negative_int32_field(
        const boost::json::object& object,
        std::string_view key
    );
    std::optional<std::vector<std::string_view>> parse_path(
        std::string_view prefix,
        std::string_view path
    );
    std::string_view get_target_path(std::string_view target);
    std::optional<std::string_view> get_target_query(std::string_view target);
    std::optional<std::vector<query_param>> parse_query_params(
        std::string_view query
    );
    std::expected<submission_dto::list_filter, response_type>
    parse_submission_list_filter_or_400(
        const request_type& request
    );
    std::expected<problem_dto::list_filter, response_type>
    parse_problem_list_filter_or_400(
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
    bool is_owner_or_admin(
        const auth_dto::identity& auth_identity_value,
        std::int64_t owner_user_id
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
    std::optional<std::string_view> get_bearer_token(
        const request_type& request
    );
}
