#pragma once

#include "error/transport_error.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>

class http_server;

class http_session : public std::enable_shared_from_this<http_session> {
public:
    using tcp = boost::asio::ip::tcp;
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    static std::expected<std::shared_ptr<http_session>, transport_error> create(
        tcp::socket socket, std::shared_ptr<http_server> http_server
    );

    std::expected<void, transport_error> run();
private:
    static constexpr std::uint64_t max_request_body_size_bytes = 32ULL * 1024ULL * 1024ULL;

    explicit http_session(tcp::socket socket, std::shared_ptr<http_server> http_server);

    static bool should_respond_to_read_error(const boost::system::error_code& ec);
    static bool should_ignore_socket_error(const boost::system::error_code& ec);

    void handle_error(transport_error code) const;
    void begin_next_request_cycle();
    void read_request();
    void wait_for_keep_alive_request();
    void on_keep_alive_socket_ready(
        std::uint64_t wait_generation,
        boost::system::error_code ec
    );
    void on_keep_alive_idle_timeout(
        std::uint64_t timer_generation,
        std::uint64_t wait_generation,
        boost::system::error_code ec
    );
    void on_request_deadline(
        std::uint64_t timer_generation,
        std::uint64_t request_generation,
        boost::system::error_code ec
    );
    void on_read(
        std::uint64_t request_generation,
        boost::system::error_code ec,
        std::size_t bytes_transferred
    );
    void on_handler_response(
        std::uint64_t request_generation,
        std::shared_ptr<response_type> response
    );
    void on_write(
        std::uint64_t request_generation,
        bool should_close,
        boost::system::error_code ec,
        std::size_t bytes_transferred
    );
    void write_response(
        std::uint64_t request_generation,
        std::shared_ptr<response_type> response
    );
    void arm_request_deadline(std::uint64_t request_generation);
    void arm_keep_alive_idle_timeout(std::uint64_t wait_generation);
    void cancel_timer();
    void cancel_socket_operations();
    void clear_active_request_state();
    std::expected<void, transport_error> close();

    request_type make_current_request_snapshot() const;
    void observe_response(
        const request_type& request,
        const response_type& response
    ) const;
    response_type create_read_error_response(
        const request_type& request,
        const boost::system::error_code& ec
    ) const;
    response_type create_deadline_exceeded_response(
        const request_type& request
    ) const;

    tcp::socket socket_;
    boost::asio::steady_timer timer_;
    boost::beast::flat_buffer buffer_;
    std::optional<boost::beast::http::request_parser<boost::beast::http::string_body>>
        request_parser_;
    std::shared_ptr<request_type> active_request_ptr_;
    std::string active_request_id_;
    std::chrono::steady_clock::time_point active_request_started_at_{};
    std::uint64_t active_request_generation_ = 0;
    std::uint64_t keep_alive_wait_generation_ = 0;
    std::uint64_t timer_generation_ = 0;
    bool session_closing_ = false;
    bool reading_in_progress_ = false;
    bool waiting_for_handler_response_ = false;
    bool write_in_progress_ = false;
    bool keep_alive_wait_in_progress_ = false;
    std::shared_ptr<http_server> http_server_;
};
