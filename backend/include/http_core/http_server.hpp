#pragma once

#include "common/db_connection_pool.hpp"
#include "common/error_code.hpp"
#include "http_core/http_dispatcher.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <memory>

class http_server{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    http_server(const http_server&) = delete;
    http_server& operator=(const http_server&) = delete;
    http_server(http_server&&) noexcept = delete;
    http_server& operator=(http_server&&) noexcept = delete;

    static std::expected<std::shared_ptr<http_server>, error_code> create();
    response_type handle(const request_type& request);
private:
    explicit http_server(db_connection_pool&& db_connection_pool);
    db_connection_pool db_connection_pool_;
    http_dispatcher http_dispatcher_;
};
