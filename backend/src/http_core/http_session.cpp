#include "http_core/http_session.hpp"

#include "common/logger.hpp"
#include "error/request_error.hpp"
#include "http_core/http_response_util.hpp"
#include "http_core/http_server.hpp"
#include "http_core/request_id_util.hpp"
#include "serializer/common_json_serializer.hpp"

#include <boost/asio/error.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/version.hpp>

#include <memory>
#include <utility>

void http_session::request_cycle_state::begin_read(read_kind next_kind){
    request_ptr.reset();
    request_id = request_id_util::make_request_id();
    started_at = std::chrono::steady_clock::now();
    generation += 1;
    kind = next_kind;
}

void http_session::request_cycle_state::set_request(request_type request){
    request_ptr = std::make_shared<request_type>(std::move(request));
}

void http_session::request_cycle_state::clear_payload(){
    request_ptr.reset();
    request_id.clear();
    started_at = {};
    kind = read_kind::initial;
}

bool http_session::request_cycle_state::matches(std::uint64_t expected_generation) const{
    return generation == expected_generation;
}

bool http_session::request_cycle_state::is_keep_alive_followup() const{
    return kind == read_kind::keep_alive_followup;
}

http_session::timer_state::timer_state(const boost::asio::any_io_executor& executor) :
    timer(executor){}

std::uint64_t http_session::timer_state::arm(
    timer_mode next_mode,
    std::chrono::milliseconds timeout
){
    timer.cancel();
    mode = next_mode;
    generation += 1;
    timer.expires_after(timeout);
    return generation;
}

void http_session::timer_state::cancel(){
    timer.cancel();
    mode = timer_mode::none;
    generation += 1;
}

bool http_session::timer_state::matches(
    timer_mode expected_mode,
    std::uint64_t expected_generation
) const{
    return mode == expected_mode && generation == expected_generation;
}

std::expected<std::shared_ptr<http_session>, transport_error> http_session::create(
    tcp::socket socket, std::shared_ptr<http_server> http_server
){
    if(!socket.is_open()){
        return std::unexpected(transport_error::bad_descriptor);
    }
    if(!http_server){
        return std::unexpected(transport_error::invalid_argument);
    }

    auto created_session = std::shared_ptr<http_session>(
        new http_session(std::move(socket), std::move(http_server))
    );
    return created_session;
}

http_session::http_session(tcp::socket socket, std::shared_ptr<http_server> http_server) :
    socket_(std::move(socket)),
    timer_state_(socket_.get_executor()),
    http_server_(std::move(http_server)){}

bool http_session::should_respond_to_read_error(const boost::system::error_code& ec){
    const auto& http_error_category =
        boost::beast::http::make_error_code(boost::beast::http::error::bad_method).category();
    return
        ec == boost::beast::http::error::body_limit ||
        ec.category() == http_error_category;
}

bool http_session::should_ignore_socket_error(const boost::system::error_code& ec){
    return
        ec == boost::asio::error::operation_aborted ||
        ec == boost::asio::error::bad_descriptor ||
        ec == boost::asio::error::not_connected;
}

std::expected<void, transport_error> http_session::run(){
    if(!socket_.is_open()){
        return std::unexpected(transport_error::bad_descriptor);
    }

    start_read(read_kind::initial);
    return {};
}

void http_session::handle_error(transport_error code) const{
    logger::cerr()
        .log("http_session_error")
        .field("code", to_string(code));
}

void http_session::start_read(read_kind next_read_kind){
    if(session_closing_){
        return;
    }

    request_parser_.emplace();
    request_parser_->body_limit(max_request_body_size_bytes);
    request_state_.begin_read(next_read_kind);
    phase_ = session_phase::reading;

    if(next_read_kind == read_kind::keep_alive_followup){
        arm_keep_alive_idle_timeout();
    }
    else{
        timer_state_.cancel();
    }

    auto self = shared_from_this();
    boost::beast::http::async_read(
        socket_,
        buffer_,
        *request_parser_,
        [self, request_generation = request_state_.generation](
            boost::system::error_code ec,
            std::size_t bytes_transferred
        ){
            self->on_read(request_generation, ec, bytes_transferred);
        }
    );
}

void http_session::on_timer_fired(
    timer_mode mode,
    std::uint64_t timer_generation,
    boost::system::error_code ec
){
    if(
        ec == boost::asio::error::operation_aborted ||
        session_closing_ ||
        !timer_state_.matches(mode, timer_generation)
    ){
        return;
    }

    timer_state_.cancel();

    switch(mode){
        case timer_mode::keep_alive_idle: {
            if(
                phase_ != session_phase::reading ||
                !request_state_.is_keep_alive_followup()
            ){
                return;
            }

            auto close_exp = close();
            if(!close_exp){
                handle_error(close_exp.error());
            }
            return;
        }
        case timer_mode::request_deadline: {
            if(!request_state_.request_ptr){
                return;
            }

            if(phase_ == session_phase::writing){
                auto close_exp = close();
                if(!close_exp){
                    handle_error(close_exp.error());
                }
                return;
            }

            if(phase_ != session_phase::awaiting_handler){
                return;
            }

            auto response = std::make_shared<response_type>(
                create_deadline_exceeded_response(*request_state_.request_ptr)
            );
            observe_response(*request_state_.request_ptr, *response);
            write_response(request_state_.generation, std::move(response));
            return;
        }
        case timer_mode::none:
            return;
    }
}

void http_session::on_read(
    std::uint64_t request_generation,
    boost::system::error_code ec,
    std::size_t bytes_transferred
){
    (void)bytes_transferred;

    if(
        phase_ != session_phase::reading ||
        !request_state_.matches(request_generation)
    ){
        return;
    }

    timer_state_.cancel();

    if(ec == boost::beast::http::error::end_of_stream){
        auto close_exp = close();
        if(!close_exp){
            handle_error(close_exp.error());
        }
        return;
    }

    if(ec){
        if(should_respond_to_read_error(ec)){
            auto response = std::make_shared<response_type>(create_read_error_response(ec));
            request_type observed_request =
                request_parser_.has_value() ? request_parser_->get() : request_type{};
            if(observed_request.version() == 0){
                observed_request.version(11);
            }
            if(observed_request.target().empty()){
                observed_request.target("/");
            }
            observed_request.keep_alive(false);
            request_parser_.reset();
            observe_response(observed_request, *response);
            write_response(request_generation, std::move(response));
            return;
        }

        if(should_ignore_socket_error(ec)){
            return;
        }

        handle_error(transport_error(ec));
        return;
    }

    request_type request = request_parser_->release();
    request_parser_.reset();
    request_state_.set_request(std::move(request));
    phase_ = session_phase::awaiting_handler;
    arm_request_deadline();

    auto self = shared_from_this();
    http_server_->async_handle(
        request_state_.request_ptr,
        request_state_.request_id,
        request_state_.started_at,
        [self, request_generation](response_type response) mutable {
            auto response_ptr = std::make_shared<response_type>(std::move(response));
            try{
                boost::asio::post(
                    self->socket_.get_executor(),
                    [self, request_generation, response_ptr = std::move(response_ptr)]() mutable {
                        self->on_handler_response(
                            request_generation,
                            std::move(response_ptr)
                        );
                    }
                );
            }
            catch(const std::bad_alloc&){
                self->handle_error(
                    transport_error{
                        transport_error_code::internal,
                        "out of memory"
                    }
                );
            }
            catch(...){
                self->handle_error(transport_error::internal);
            }
        }
    );
}

void http_session::on_handler_response(
    std::uint64_t request_generation,
    std::shared_ptr<response_type> response
){
    if(
        session_closing_ ||
        phase_ != session_phase::awaiting_handler ||
        !request_state_.matches(request_generation)
    ){
        return;
    }

    write_response(request_generation, std::move(response));
}

void http_session::on_write(
    std::uint64_t request_generation,
    bool should_close,
    boost::system::error_code ec,
    std::size_t bytes_transferred
){
    (void)bytes_transferred;

    if(
        phase_ != session_phase::writing ||
        !request_state_.matches(request_generation)
    ){
        return;
    }

    timer_state_.cancel();

    if(ec){
        if(should_ignore_socket_error(ec)){
            return;
        }

        handle_error(transport_error(ec));
        return;
    }

    if(should_close){
        auto close_exp = close();
        if(!close_exp){
            handle_error(close_exp.error());
        }
        return;
    }

    start_read(read_kind::keep_alive_followup);
}

void http_session::write_response(
    std::uint64_t request_generation,
    std::shared_ptr<response_type> response
){
    if(session_closing_){
        return;
    }

    phase_ = session_phase::writing;
    const bool should_close = response->need_eof();

    auto self = shared_from_this();
    boost::beast::http::async_write(
        socket_,
        *response,
        [self, response, request_generation, should_close](
            boost::system::error_code ec,
            std::size_t write_bytes
        ){
            self->on_write(request_generation, should_close, ec, write_bytes);
        }
    );
}

void http_session::arm_request_deadline(){
    const auto deadline_opt = http_server_->runtime_config().request_deadline_opt;
    if(!deadline_opt){
        timer_state_.cancel();
        return;
    }

    const std::uint64_t timer_generation =
        timer_state_.arm(timer_mode::request_deadline, *deadline_opt);
    auto self = shared_from_this();
    timer_state_.timer.async_wait(
        [self, timer_generation](boost::system::error_code ec){
            self->on_timer_fired(
                timer_mode::request_deadline,
                timer_generation,
                ec
            );
        }
    );
}

void http_session::arm_keep_alive_idle_timeout(){
    const auto timeout_opt = http_server_->runtime_config().keep_alive_idle_timeout_opt;
    if(!timeout_opt){
        timer_state_.cancel();
        return;
    }

    const std::uint64_t timer_generation =
        timer_state_.arm(timer_mode::keep_alive_idle, *timeout_opt);
    auto self = shared_from_this();
    timer_state_.timer.async_wait(
        [self, timer_generation](boost::system::error_code ec){
            self->on_timer_fired(
                timer_mode::keep_alive_idle,
                timer_generation,
                ec
            );
        }
    );
}

void http_session::cancel_socket_operations(){
    boost::system::error_code ec;
    socket_.cancel(ec);
}

std::expected<void, transport_error> http_session::close(){
    if(session_closing_){
        return {};
    }

    session_closing_ = true;
    phase_ = session_phase::closed;
    timer_state_.cancel();
    cancel_socket_operations();
    request_parser_.reset();
    request_state_.clear_payload();

    boost::system::error_code shutdown_ec;
    socket_.shutdown(tcp::socket::shutdown_both, shutdown_ec);
    if(
        shutdown_ec &&
        shutdown_ec != boost::asio::error::not_connected &&
        shutdown_ec != boost::asio::error::bad_descriptor
    ){
        return std::unexpected(transport_error(shutdown_ec));
    }

    boost::system::error_code close_ec;
    socket_.close(close_ec);
    if(
        close_ec &&
        close_ec != boost::asio::error::not_connected &&
        close_ec != boost::asio::error::bad_descriptor
    ){
        return std::unexpected(transport_error(close_ec));
    }

    return {};
}

void http_session::observe_response(
    const request_type& request,
    const response_type& response
) const{
    if(request_state_.request_id.empty() || !http_server_){
        return;
    }

    http_server_->observe_request_completion(
        request,
        request_state_.request_id,
        response,
        std::chrono::steady_clock::now() - request_state_.started_at
    );
}

http_session::response_type http_session::create_read_error_response(
    const boost::system::error_code& ec
) const{
    const auto error = ec == boost::beast::http::error::body_limit
        ? request_error::make_payload_too_large_error()
        : request_error::make_bad_request_error("bad request: " + ec.message());
    const unsigned http_version =
        request_parser_.has_value() && request_parser_->get().version() != 0
            ? request_parser_->get().version()
            : 11;

    response_type response{error.status(), http_version};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(
        boost::beast::http::field::content_type,
        "application/json; charset=utf-8"
    );
    response.keep_alive(false);
    response.body() = boost::json::serialize(
        common_json_serializer::make_error_object(error.code_string(), error.message)
    ) + "\n";
    response.prepare_payload();
    request_id_util::set_response_header(response, request_state_.request_id);
    return response;
}

http_session::response_type http_session::create_deadline_exceeded_response(
    const request_type& request
) const{
    request_type request_snapshot = request;
    if(request_snapshot.version() == 0){
        request_snapshot.version(11);
    }
    request_snapshot.keep_alive(false);

    auto response = http_response_util::create_service_unavailable(
        request_snapshot,
        "request deadline exceeded, retry later"
    );
    response.keep_alive(false);
    request_id_util::set_response_header(response, request_state_.request_id);
    return response;
}
