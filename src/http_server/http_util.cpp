#include "http_server/http_util.hpp"

#include <utility>

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
