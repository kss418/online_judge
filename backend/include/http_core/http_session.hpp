#pragma once

#include "error/error_code.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>

class http_server;

class http_session : public std::enable_shared_from_this<http_session> {
public:
    using tcp = boost::asio::ip::tcp;
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    static std::expected<std::shared_ptr<http_session>, error_code> create(
        tcp::socket socket, std::shared_ptr<http_server> http_server
    );

    std::expected<void, error_code> run();
private:
    static constexpr std::uint64_t max_request_body_size_bytes = 32ULL * 1024ULL * 1024ULL;

    explicit http_session(tcp::socket socket, std::shared_ptr<http_server> http_server);

    static bool should_respond_to_read_error(const boost::system::error_code& ec);
    void handle_error(error_code code) const;
    void read();
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
    void on_write(bool should_close, boost::system::error_code ec, std::size_t bytes_transferred);
    void write_response(std::shared_ptr<response_type> response);
    std::expected<void, error_code> close();

    response_type create_read_error_response(const boost::system::error_code& ec) const;

    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    std::optional<boost::beast::http::request_parser<boost::beast::http::string_body>>
        request_parser_;
    std::shared_ptr<http_server> http_server_;
};
