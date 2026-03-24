#pragma once

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

class system_handler{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type handle_health_get(const request_type& request);
    response_type handle_supported_languages_get(const request_type& request);
};
