#include "http_server/http_util.hpp"

#include "common/string_util.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include <boost/beast/core/string.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/version.hpp>

http_util::response_type http_util::create_text_response(
    const request_type& request,
    boost::beast::http::status status,
    std::string body
){
    response_type response{status, request.version()};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "text/plain; charset=utf-8");
    response.keep_alive(request.keep_alive());
    response.body() = std::move(body);
    response.prepare_payload();
    return response;
}

http_util::response_type http_util::create_400_or_500_response(
    const request_type& request,
    std::string_view action,
    const error_code& code
){
    const auto status = code.is_bad_request_error()
        ? boost::beast::http::status::bad_request
        : boost::beast::http::status::internal_server_error;

    return create_text_response(
        request,
        status,
        "failed to " + std::string{action} + ": " + to_string(code) + "\n"
    );
}

http_util::response_type http_util::create_404_or_500_response(
    const request_type& request,
    std::string_view action,
    const error_code& code
){
    const auto status = code.is_bad_request_error()
        ? boost::beast::http::status::not_found
        : boost::beast::http::status::internal_server_error;

    return create_text_response(
        request,
        status,
        "failed to " + std::string{action} + ": " + to_string(code) + "\n"
    );
}

http_util::response_type http_util::create_bearer_unauthorized_response(
    const request_type& request,
    std::string body
){
    auto response = create_text_response(
        request,
        boost::beast::http::status::unauthorized,
        std::move(body)
    );
    response.set(boost::beast::http::field::www_authenticate, "Bearer");
    return response;
}

http_util::response_type http_util::method_not_allowed_response(
    const request_type& request
){
    return create_text_response(
        request,
        boost::beast::http::status::method_not_allowed,
        "method not allowed\n"
    );
}

http_util::response_type http_util::not_found_response(
    const request_type& request
){
    return create_text_response(
        request,
        boost::beast::http::status::not_found,
        "not found\n"
    );
}

std::optional<boost::json::object> http_util::parse_json_object(
    const request_type& request
){
    boost::system::error_code ec;
    auto request_value = boost::json::parse(request.body(), ec);
    if(ec || !request_value.is_object()){
        return std::nullopt;
    }

    return std::move(request_value.as_object());
}

std::optional<std::string_view> http_util::get_string_field(
    const boost::json::object& object,
    std::string_view key
){
    const auto* value = object.if_contains(key);
    if(!value || !value->is_string()){
        return std::nullopt;
    }

    const auto& string_value = value->as_string();
    return std::string_view{string_value.data(), string_value.size()};
}

std::optional<std::string_view> http_util::get_non_empty_string_field(
    const boost::json::object& object,
    std::string_view key
){
    const auto string_value_opt = get_string_field(object, key);
    if(!string_value_opt || string_value_opt->empty()){
        return std::nullopt;
    }

    return string_value_opt;
}

std::optional<std::int64_t> http_util::get_positive_int64_field(
    const boost::json::object& object,
    std::string_view key
){
    const auto* value = object.if_contains(key);
    if(value == nullptr){
        return std::nullopt;
    }

    if(value->is_int64()){
        const std::int64_t int64_value = value->as_int64();
        if(int64_value <= 0){
            return std::nullopt;
        }

        return int64_value;
    }

    if(value->is_uint64()){
        const std::uint64_t uint64_value = value->as_uint64();
        if(
            uint64_value == 0 ||
            uint64_value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
        ){
            return std::nullopt;
        }

        return static_cast<std::int64_t>(uint64_value);
    }

    return std::nullopt;
}

std::optional<std::int32_t> http_util::get_positive_int32_field(
    const boost::json::object& object,
    std::string_view key
){
    const auto int64_value_opt = get_positive_int64_field(object, key);
    if(
        !int64_value_opt ||
        *int64_value_opt > std::numeric_limits<std::int32_t>::max()
    ){
        return std::nullopt;
    }

    return static_cast<std::int32_t>(*int64_value_opt);
}

std::optional<std::vector<std::string_view>> http_util::parse_path(
    std::string_view prefix,
    std::string_view path
){
    if(!path.starts_with(prefix)){
        return std::nullopt;
    }

    path.remove_prefix(prefix.size());
    if(!path.empty() && path.front() != '/'){
        return std::nullopt;
    }
    if(path.starts_with("/")){
        path.remove_prefix(1);
    }

    std::vector<std::string_view> path_segments;
    while(!path.empty()){
        const std::size_t slash_position = path.find('/');
        if(slash_position == std::string_view::npos){
            path_segments.push_back(path);
            break;
        }

        path_segments.push_back(path.substr(0, slash_position));
        path.remove_prefix(slash_position + 1);
    }

    return path_segments;
}

std::string_view http_util::get_target_path(std::string_view target){
    const auto query_position = target.find('?');
    if(query_position == std::string_view::npos){
        return target;
    }

    return target.substr(0, query_position);
}

std::optional<std::string_view> http_util::get_target_query(std::string_view target){
    const auto query_position = target.find('?');
    if(query_position == std::string_view::npos){
        return std::nullopt;
    }

    return target.substr(query_position + 1);
}

std::optional<std::vector<http_util::query_param>> http_util::parse_query_params(
    std::string_view query
){
    std::vector<query_param> query_params;
    if(query.empty()){
        return query_params;
    }

    while(true){
        const auto ampersand_position = query.find('&');
        const auto query_segment =
            ampersand_position == std::string_view::npos
                ? query
                : query.substr(0, ampersand_position);

        const auto equal_position = query_segment.find('=');
        if(
            query_segment.empty() ||
            equal_position == std::string_view::npos ||
            equal_position == 0 ||
            equal_position + 1 >= query_segment.size()
        ){
            return std::nullopt;
        }

        query_params.push_back(query_param{
            .key = query_segment.substr(0, equal_position),
            .value = query_segment.substr(equal_position + 1)
        });

        if(ampersand_position == std::string_view::npos){
            break;
        }

        query.remove_prefix(ampersand_position + 1);
    }

    return query_params;
}

std::expected<auth_dto::token, http_util::response_type> http_util::parse_bearer_token_or_401(
    const request_type& request
){
    const auto token_opt = get_bearer_token(request);
    if(!token_opt){
        return std::unexpected(create_bearer_unauthorized_response(
            request,
            "missing or invalid bearer token\n"
        ));
    }

    auth_dto::token token_value;
    token_value.value = std::string{*token_opt};
    return token_value;
}

std::expected<auth_dto::identity, http_util::response_type> http_util::try_auth_bearer(
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
            return std::unexpected(create_bearer_unauthorized_response(
                request,
                "missing or invalid bearer token\n"
            ));
        }

        return std::unexpected(create_text_response(
            request,
            boost::beast::http::status::internal_server_error,
            "failed to authenticate token: " + to_string(code) + "\n"
        ));
    }
    if(!auth_identity_exp->has_value()){
        return std::unexpected(create_bearer_unauthorized_response(
            request,
            "invalid, expired, or revoked token\n"
        ));
    }

    return auth_identity_exp->value();
}

std::expected<auth_dto::identity, http_util::response_type> http_util::try_admin_auth_bearer(
    const request_type& request,
    db_connection& db_connection
){
    const auto auth_identity_exp = try_auth_bearer(request, db_connection);
    if(!auth_identity_exp){
        return std::unexpected(std::move(auth_identity_exp.error()));
    }
    if(!auth_identity_exp->is_admin){
        return std::unexpected(create_bearer_unauthorized_response(
            request,
            "admin bearer token required\n"
        ));
    }

    return *auth_identity_exp;
}

std::optional<std::string_view> http_util::get_bearer_token(
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
