#pragma once

#include "common/error_code.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <expected>
#include <memory>

class http_session : public std::enable_shared_from_this<http_session> {
public:
    using tcp = boost::asio::ip::tcp;
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    static std::expected<std::shared_ptr<http_session>, error_code> create(tcp::socket socket);

    std::expected<void, error_code> run();

private:
    explicit http_session(tcp::socket socket);

    void handle_error(error_code code) const;
    void read();
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
    void on_write(bool should_close, boost::system::error_code ec, std::size_t bytes_transferred);
    std::expected<void, error_code> close();

    response_type create_response() const;

    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    request_type request_;
};
