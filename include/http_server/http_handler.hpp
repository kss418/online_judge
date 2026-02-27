#pragma once

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

namespace http_handler{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    response_type handle(const request_type& request);
}
