#pragma once
#include "error/transport_error.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <expected>
#include <memory>

class http_server;

class acceptor : public std::enable_shared_from_this<acceptor>{
public:
    using tcp = boost::asio::ip::tcp;
    static std::expected<std::shared_ptr<acceptor>, transport_error> create(
        boost::asio::io_context& io_context,
        const tcp::endpoint& endpoint,
        std::shared_ptr<http_server> http_server
    );

    std::expected<void, transport_error> run();
private:
    explicit acceptor(boost::asio::io_context& io_context, std::shared_ptr<http_server> http_server);

    std::expected<void, transport_error> initialize(const tcp::endpoint& endpoint);
    void handle_error(transport_error code) const;
    void accept();
    void on_accept(boost::system::error_code ec, tcp::socket socket);

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    std::shared_ptr <http_server> http_server_;
};
