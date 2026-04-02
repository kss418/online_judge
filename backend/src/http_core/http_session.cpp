#include "http_core/http_session.hpp"
#include "common/logger.hpp"
#include "http_core/http_server.hpp"
#include "http_core/http_response_util.hpp"
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
    http_server_(std::move(http_server)){}

bool http_session::should_respond_to_read_error(const boost::system::error_code& ec){
    const auto& http_error_category =
        boost::beast::http::make_error_code(boost::beast::http::error::bad_method).category();
    return
        ec == boost::beast::http::error::body_limit ||
        ec.category() == http_error_category;
}

std::expected<void, transport_error> http_session::run(){
    if(!socket_.is_open()){
        return std::unexpected(transport_error::bad_descriptor);
    }

    read();
    return {};
}

void http_session::handle_error(transport_error code) const{
    logger::cerr()
        .log("http_session_error")
        .field("code", to_string(code));
}

void http_session::read(){
    request_parser_.emplace();
    request_parser_->body_limit(max_request_body_size_bytes);

    auto self = shared_from_this();
    boost::beast::http::async_read(
        socket_,
        buffer_,
        *request_parser_,
        [self](boost::system::error_code ec, std::size_t bytes_transferred){
            self->on_read(ec, bytes_transferred);
        }
    );
}

void http_session::on_read(boost::system::error_code ec, std::size_t bytes_transferred){
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
            request_parser_.reset();
            write_response(response);
            return;
        }

        handle_error(transport_error(ec));
        return;
    }

    request_type request = request_parser_->release();
    request_parser_.reset();

    auto self = shared_from_this();
    http_server_->async_handle(
        std::move(request),
        [self](response_type response) mutable {
            auto response_ptr = std::make_shared<response_type>(std::move(response));
            try{
                boost::asio::post(
                    self->socket_.get_executor(),
                    [self, response_ptr = std::move(response_ptr)]() mutable {
                        self->write_response(std::move(response_ptr));
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

void http_session::on_write(
    bool should_close, boost::system::error_code ec, std::size_t bytes_transferred
){
    if(ec){
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

    read();
}

void http_session::write_response(std::shared_ptr<response_type> response){
    const bool should_close = response->need_eof();

    auto self = shared_from_this();
    boost::beast::http::async_write(
        socket_,
        *response,
        [self, response, should_close](boost::system::error_code ec, std::size_t write_bytes){
            self->on_write(should_close, ec, write_bytes);
        }
    );
}

std::expected<void, transport_error> http_session::close(){
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_send, ec);

    if(ec && ec != boost::asio::error::not_connected){
        return std::unexpected(transport_error(ec));
    }

    return {};
}

http_session::response_type http_session::create_read_error_response(
    const boost::system::error_code& ec
) const{
    const auto status = ec == boost::beast::http::error::body_limit
        ? boost::beast::http::status::payload_too_large
        : boost::beast::http::status::bad_request;
    const auto error_code_text = ec == boost::beast::http::error::body_limit
        ? "payload_too_large"
        : "bad_request";
    const std::string error_message = ec == boost::beast::http::error::body_limit
        ? "request body too large"
        : "bad request: " + ec.message();
    const unsigned http_version =
        request_parser_ && request_parser_->get().version() != 0
            ? request_parser_->get().version()
            : 11;

    response_type response{status, http_version};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(
        boost::beast::http::field::content_type,
        "application/json; charset=utf-8"
    );
    response.keep_alive(false);
    response.body() = boost::json::serialize(
        common_json_serializer::make_error_object(error_code_text, error_message)
    ) + "\n";
    response.prepare_payload();
    return response;
}
