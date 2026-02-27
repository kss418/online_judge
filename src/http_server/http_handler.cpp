#include "http_server/http_handler.hpp"

#include <array>
#include <string>
#include <string_view>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/version.hpp>

http_handler::response_type http_handler::create_text_response(
    const request_type& request, boost::beast::http::status status, std::string body
){
    http_handler::response_type response{status, request.version()};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "text/plain; charset=utf-8");
    response.keep_alive(request.keep_alive());
    response.body() = std::move(body);
    response.prepare_payload();
    return response;
}

http_handler::response_type http_handler::handle_health_get(const request_type& request){
    return create_text_response(request, boost::beast::http::status::ok, "ok\n");
}

http_handler::response_type http_handler::handle(const request_type& request){
    for(const auto& entry : routes){
        if(entry.method_ == request.method() && entry.target_ == request.target()){
            return entry.handler_(request);
        }
    }

    for(const auto& entry : routes){
        if(entry.target_ == request.target()){
            return create_text_response(
                request, boost::beast::http::status::method_not_allowed, "method not allowed\n"
            );
        }
    }

    return create_text_response(request, boost::beast::http::status::not_found, "not found\n");
}
