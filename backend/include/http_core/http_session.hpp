#pragma once

#include "error/transport_error.hpp"

#include <boost/asio/any_io_executor.hpp>
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
    enum class read_kind{
        initial,
        keep_alive_followup
    };
    enum class session_phase{
        reading,
        awaiting_handler,
        writing,
        closed
    };
    enum class timer_mode{
        none,
        keep_alive_idle,
        request_deadline
    };

    struct request_cycle_state{
        void begin_read(read_kind next_kind);
        void set_request(request_type request);
        void clear_payload();
        bool matches(std::uint64_t expected_generation) const;
        bool is_keep_alive_followup() const;

        std::shared_ptr<request_type> request_ptr;
        std::string request_id;
        std::chrono::steady_clock::time_point started_at{};
        std::uint64_t generation = 0;
        read_kind kind = read_kind::initial;
    };

    struct timer_state{
        explicit timer_state(const boost::asio::any_io_executor& executor);

        std::uint64_t arm(timer_mode next_mode, std::chrono::milliseconds timeout);
        void cancel();
        bool matches(timer_mode expected_mode, std::uint64_t expected_generation) const;

        boost::asio::steady_timer timer;
        timer_mode mode = timer_mode::none;
        std::uint64_t generation = 0;
    };

    explicit http_session(tcp::socket socket, std::shared_ptr<http_server> http_server);

    static bool should_respond_to_read_error(const boost::system::error_code& ec);
    static bool should_ignore_socket_error(const boost::system::error_code& ec);

    void handle_error(transport_error code) const;
    void start_read(read_kind next_read_kind);
    void on_timer_fired(
        timer_mode mode,
        std::uint64_t timer_generation,
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
    void arm_request_deadline();
    void arm_keep_alive_idle_timeout();
    void cancel_socket_operations();
    std::expected<void, transport_error> close();

    void observe_response(
        const request_type& request,
        const response_type& response
    ) const;
    response_type create_read_error_response(const boost::system::error_code& ec) const;
    response_type create_deadline_exceeded_response(
        const request_type& request
    ) const;

    tcp::socket socket_;
    timer_state timer_state_;
    boost::beast::flat_buffer buffer_;
    std::optional<boost::beast::http::request_parser<boost::beast::http::string_body>>
        request_parser_;
    request_cycle_state request_state_;
    session_phase phase_ = session_phase::closed;
    bool session_closing_ = false;
    std::shared_ptr<http_server> http_server_;
};
