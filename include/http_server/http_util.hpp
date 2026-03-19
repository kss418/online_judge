#pragma once

#include "db_service/auth_service.hpp"
#include "common/db_connection.hpp"
#include "dto/auth_dto.hpp"
#include "dto/dto_validation_error.hpp"

#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace http_util{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    struct query_param{
        std::string_view key;
        std::string_view value;
    };

    response_type create_text_response(
        const request_type& request,
        boost::beast::http::status status,
        std::string body
    );
    response_type create_400_or_500_response(
        const request_type& request,
        std::string_view action,
        const error_code& code
    );
    response_type create_404_or_500_response(
        const request_type& request,
        std::string_view action,
        const error_code& code
    );
    response_type create_bearer_unauthorized_response(
        const request_type& request,
        std::string body
    );
    response_type method_not_allowed_response(
        const request_type& request
    );
    response_type not_found_response(
        const request_type& request
    );
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
            return std::unexpected(create_text_response(
                request,
                boost::beast::http::status::bad_request,
                "invalid json\n"
            ));
        }

        auto dto_exp = std::invoke(
            std::forward<factory_type>(factory),
            *request_object_opt,
            std::forward<arg_types>(args)...
        );
        if(!dto_exp){
            return std::unexpected(create_text_response(
                request,
                boost::beast::http::status::bad_request,
                dto_exp.error().message + "\n"
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
    std::optional<std::vector<std::string_view>> parse_path(
        std::string_view prefix,
        std::string_view path
    );
    std::string_view get_target_path(std::string_view target);
    std::optional<std::string_view> get_target_query(std::string_view target);
    std::optional<std::vector<query_param>> parse_query_params(
        std::string_view query
    );
    std::expected<auth_dto::identity, response_type> try_auth_bearer(
        const request_type& request,
        db_connection& db_connection
    );
    std::expected<auth_dto::identity, response_type> try_admin_auth_bearer(
        const request_type& request,
        db_connection& db_connection
    );
    std::optional<std::string_view> get_bearer_token(
        const request_type& request
    );
}
