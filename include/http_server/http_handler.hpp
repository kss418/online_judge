#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <string>

class http_handler{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    http_handler(const http_handler&) = delete;
    http_handler& operator=(const http_handler&) = delete;
    http_handler(http_handler&&) noexcept = default;
    http_handler& operator=(http_handler&&) noexcept = default;

    static std::expected<http_handler, error_code> create(db_connection db_connection);
    response_type handle_health_get(const request_type& request);

private:
    explicit http_handler(db_connection db_connection);

    db_connection db_connection_;
};
