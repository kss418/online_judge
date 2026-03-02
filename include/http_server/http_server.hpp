#pragma once

#include "common/error_code.hpp"
#include "http_server/http_handler.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <memory>

class acceptor;
class http_server : public std::enable_shared_from_this<http_server>{
public:
    using tcp = boost::asio::ip::tcp;
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    static std::expected<std::shared_ptr<http_server>, error_code> create(
        boost::asio::io_context& io_context,
        const tcp::endpoint& endpoint
    );

    std::expected<void, error_code> run();
    response_type handle(const request_type& request);

private:
    explicit http_server(
        boost::asio::io_context& io_context,
        http_handler http_handler
    );

    std::expected<void, error_code> initialize(const tcp::endpoint& endpoint);

    boost::asio::io_context& io_context_;
    http_handler http_handler_;
    std::shared_ptr<acceptor> acceptor_;
};
