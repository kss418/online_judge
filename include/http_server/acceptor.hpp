#pragma once
#include "common/error_code.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <expected>
#include <memory>

class acceptor : public std::enable_shared_from_this<acceptor> {
public:
    using tcp = boost::asio::ip::tcp;
    static std::expected<std::shared_ptr<acceptor>, error_code> create(
        boost::asio::io_context& io_context, const tcp::endpoint& endpoint
    );

    std::expected<void, error_code> run();
private:
    explicit acceptor(boost::asio::io_context& io_context);

    std::expected<void, error_code> initialize(const tcp::endpoint& endpoint);
    void handle_error(error_code code) const;
    void accept();
    void on_accept(boost::system::error_code ec, tcp::socket socket);

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
};
