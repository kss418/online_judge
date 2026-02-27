#include "http_server/http_handler.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/version.hpp>

http_handler::response_type http_handler::handle(const request_type& request){
    http_handler::response_type response{boost::beast::http::status::ok, request.version()};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "text/plain; charset=utf-8");
    response.keep_alive(request.keep_alive());

    if(request.method() != boost::beast::http::verb::get){
        response.result(boost::beast::http::status::method_not_allowed);
        response.body() = "method not allowed\n";
        response.prepare_payload();
        return response;
    }

    if(request.target() == "/health"){
        response.body() = "ok\n";
        response.prepare_payload();
        return response;
    }

    response.body() = "online_judge http server\n";
    response.prepare_payload();
    return response;
}
