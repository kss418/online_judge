#include "http_server/http_util.hpp"

#include "common/string_util.hpp"

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

std::optional<std::string_view> http_util::get_non_empty_string_field(
    const boost::json::object& object,
    std::string_view key
){
    const auto* value = object.if_contains(key);
    if(!value || !value->is_string()){
        return std::nullopt;
    }

    const auto& string_value = value->as_string();
    if(string_value.empty()){
        return std::nullopt;
    }

    return std::string_view{string_value.data(), string_value.size()};
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
