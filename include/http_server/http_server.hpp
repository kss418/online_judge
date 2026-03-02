#pragma once

#include "common/error_code.hpp"
#include "http_server/http_handler.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <memory>

class http_server{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    static std::expected<std::shared_ptr<http_server>, error_code> create();
    response_type handle(const request_type& request);
private:
    explicit http_server(http_handler http_handler);
    http_handler http_handler_;
};
