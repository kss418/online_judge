#pragma once

#include <array>
#include <string>
#include <string_view>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>

namespace http_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;
    using handler_fn = response_type(*)(const request_type& request);

    struct route{
        boost::beast::http::verb method_;
        std::string_view target_;
        handler_fn handler_;
    };

    response_type create_text_response(
        const request_type& request, boost::beast::http::status status, std::string body
    );

    response_type handle_health_get(const request_type& request);

    inline const std::array routes{
        route{boost::beast::http::verb::get, "/health", &handle_health_get},
    };

    response_type handle(const request_type& request);
}
