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
    timer_(socket_.get_executor()),
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

    begin_next_request_cycle();
    return {};
}

void http_session::handle_error(transport_error code) const{
    logger::cerr()
        .log("http_session_error")
        .field("code", to_string(code));
}

void http_session::begin_next_request_cycle(){
    if(session_closing_){
        return;
    }

    clear_active_request_state();
    if(buffer_.size() > 0 || !http_server_->runtime_config().keep_alive_idle_timeout_opt){
        read_request();
        return;
    }

    wait_for_keep_alive_request();
}

void http_session::read_request(){
    if(session_closing_){
        return;
    }

    keep_alive_wait_in_progress_ = false;
    reading_in_progress_ = true;
    active_request_generation_ += 1;
    active_request_id_ = request_id_util::make_request_id();
    active_request_started_at_ = std::chrono::steady_clock::now();
    request_parser_.emplace();
    request_parser_->body_limit(max_request_body_size_bytes);
    arm_request_deadline(active_request_generation_);

    auto self = shared_from_this();
    boost::beast::http::async_read(
        socket_,
        buffer_,
        *request_parser_,
        [self, request_generation = active_request_generation_](
            boost::system::error_code ec,
            std::size_t bytes_transferred
        ){
            self->on_read(request_generation, ec, bytes_transferred);
        }
    );
}

void http_session::wait_for_keep_alive_request(){
    if(session_closing_){
        return;
    }

    keep_alive_wait_in_progress_ = true;
    keep_alive_wait_generation_ += 1;
    arm_keep_alive_idle_timeout(keep_alive_wait_generation_);

    auto self = shared_from_this();
    socket_.async_wait(
        tcp::socket::wait_read,
        [self, wait_generation = keep_alive_wait_generation_](
            boost::system::error_code ec
        ){
            self->on_keep_alive_socket_ready(wait_generation, ec);
        }
    );
}

void http_session::on_keep_alive_socket_ready(
    std::uint64_t wait_generation,
    boost::system::error_code ec
){
    if(wait_generation != keep_alive_wait_generation_ || !keep_alive_wait_in_progress_){
        return;
    }

    keep_alive_wait_in_progress_ = false;
    cancel_timer();

    if(ec){
        if(should_ignore_socket_error(ec)){
            return;
        }

        handle_error(transport_error(ec));
        return;
    }

    read_request();
}

void http_session::on_keep_alive_idle_timeout(
    std::uint64_t timer_generation,
    std::uint64_t wait_generation,
    boost::system::error_code ec
){
    if(
        ec == boost::asio::error::operation_aborted ||
        timer_generation != timer_generation_ ||
        wait_generation != keep_alive_wait_generation_ ||
        !keep_alive_wait_in_progress_
    ){
        return;
    }

    keep_alive_wait_in_progress_ = false;
    auto close_exp = close();
    if(!close_exp){
        handle_error(close_exp.error());
    }
}

void http_session::on_request_deadline(
    std::uint64_t timer_generation,
    std::uint64_t request_generation,
    boost::system::error_code ec
){
    if(
        ec == boost::asio::error::operation_aborted ||
        timer_generation != timer_generation_ ||
        request_generation != active_request_generation_ ||
        session_closing_
    ){
        return;
    }

    cancel_timer();

    if(write_in_progress_){
        auto close_exp = close();
        if(!close_exp){
            handle_error(close_exp.error());
        }
        return;
    }

    const request_type request_snapshot = make_current_request_snapshot();
    const auto response = std::make_shared<response_type>(
        create_deadline_exceeded_response(request_snapshot)
    );

    if(reading_in_progress_ || keep_alive_wait_in_progress_){
        cancel_socket_operations();
    }

    request_parser_.reset();
    active_request_ptr_.reset();
    reading_in_progress_ = false;
    keep_alive_wait_in_progress_ = false;
    waiting_for_handler_response_ = false;

    observe_response(request_snapshot, *response);
    write_response(request_generation, response);
}

void http_session::on_read(
    std::uint64_t request_generation,
    boost::system::error_code ec,
    std::size_t bytes_transferred
){
    (void)bytes_transferred;

    if(request_generation != active_request_generation_){
        return;
    }

    reading_in_progress_ = false;

    if(ec == boost::beast::http::error::end_of_stream){
        auto close_exp = close();
        if(!close_exp){
            handle_error(close_exp.error());
        }
        return;
    }

    if(ec){
        if(should_respond_to_read_error(ec)){
            const request_type request_snapshot = make_current_request_snapshot();
            auto response = std::make_shared<response_type>(
                create_read_error_response(request_snapshot, ec)
            );
            request_parser_.reset();
            observe_response(request_snapshot, *response);
            write_response(request_generation, response);
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
    active_request_ptr_ = std::make_shared<request_type>(std::move(request));
    waiting_for_handler_response_ = true;

    auto self = shared_from_this();
    http_server_->async_handle(
        active_request_ptr_,
        active_request_id_,
        active_request_started_at_,
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
        request_generation != active_request_generation_ ||
        !waiting_for_handler_response_ ||
        session_closing_
    ){
        return;
    }

    waiting_for_handler_response_ = false;
    write_response(request_generation, std::move(response));
}

void http_session::on_write(
    std::uint64_t request_generation,
    bool should_close,
    boost::system::error_code ec,
    std::size_t bytes_transferred
){
    (void)bytes_transferred;

    if(request_generation != active_request_generation_ && active_request_generation_ != 0){
        return;
    }

    write_in_progress_ = false;
    cancel_timer();

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

    begin_next_request_cycle();
}

void http_session::write_response(
    std::uint64_t request_generation,
    std::shared_ptr<response_type> response
){
    if(session_closing_){
        return;
    }

    write_in_progress_ = true;
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

void http_session::arm_request_deadline(std::uint64_t request_generation){
    const auto deadline_opt = http_server_->runtime_config().request_deadline_opt;
    if(!deadline_opt){
        return;
    }

    cancel_timer();
    timer_.expires_after(*deadline_opt);

    auto self = shared_from_this();
    const std::uint64_t timer_generation = timer_generation_;
    timer_.async_wait(
        [self, timer_generation, request_generation](boost::system::error_code ec){
            self->on_request_deadline(timer_generation, request_generation, ec);
        }
    );
}

void http_session::arm_keep_alive_idle_timeout(std::uint64_t wait_generation){
    const auto timeout_opt = http_server_->runtime_config().keep_alive_idle_timeout_opt;
    if(!timeout_opt){
        return;
    }

    cancel_timer();
    timer_.expires_after(*timeout_opt);

    auto self = shared_from_this();
    const std::uint64_t timer_generation = timer_generation_;
    timer_.async_wait(
        [self, timer_generation, wait_generation](boost::system::error_code ec){
            self->on_keep_alive_idle_timeout(timer_generation, wait_generation, ec);
        }
    );
}

void http_session::cancel_timer(){
    timer_generation_ += 1;
    timer_.cancel();
}

void http_session::cancel_socket_operations(){
    boost::system::error_code ec;
    socket_.cancel(ec);
}

void http_session::clear_active_request_state(){
    request_parser_.reset();
    active_request_ptr_.reset();
    active_request_id_.clear();
    active_request_started_at_ = {};
    reading_in_progress_ = false;
    waiting_for_handler_response_ = false;
    write_in_progress_ = false;
    keep_alive_wait_in_progress_ = false;
}

std::expected<void, transport_error> http_session::close(){
    if(session_closing_){
        return {};
    }

    session_closing_ = true;
    cancel_timer();
    cancel_socket_operations();
    clear_active_request_state();

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

http_session::request_type http_session::make_current_request_snapshot() const{
    if(active_request_ptr_){
        return *active_request_ptr_;
    }

    if(request_parser_){
        return request_parser_->get();
    }

    request_type request;
    request.version(11);
    request.keep_alive(false);
    request.target("/");
    return request;
}

void http_session::observe_response(
    const request_type& request,
    const response_type& response
) const{
    if(active_request_id_.empty() || !http_server_){
        return;
    }

    http_server_->observe_request_completion(
        request,
        active_request_id_,
        response,
        std::chrono::steady_clock::now() - active_request_started_at_
    );
}

http_session::response_type http_session::create_read_error_response(
    const request_type& request,
    const boost::system::error_code& ec
) const{
    const auto error = ec == boost::beast::http::error::body_limit
        ? request_error::make_payload_too_large_error()
        : request_error::make_bad_request_error("bad request: " + ec.message());

    const unsigned http_version = request.version() != 0 ? request.version() : 11;

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
    request_id_util::set_response_header(response, active_request_id_);
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
    request_id_util::set_response_header(response, active_request_id_);
    return response;
}
