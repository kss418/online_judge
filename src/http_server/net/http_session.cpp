#include "http_server/net/http_session.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/version.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <utility>

std::expected<std::shared_ptr<http_session>, error_code> http_session::create(tcp::socket socket){
    if(!socket.is_open()){
        return std::unexpected(error_code::create(boost_error::bad_descriptor));
    }

    auto created_session = std::shared_ptr<http_session>(new http_session(std::move(socket)));
    return created_session;
}

http_session::http_session(tcp::socket socket) : socket_(std::move(socket)){}

std::expected<void, error_code> http_session::run(){
    if(!socket_.is_open()){
        return std::unexpected(error_code::create(boost_error::bad_descriptor));
    }

    read();
    return {};
}

void http_session::handle_error(error_code code) const{
    std::cerr << "http_session error: " << to_string(code) << '\n';
}

void http_session::read(){
    request_ = {};

    auto self = shared_from_this();
    boost::beast::http::async_read(
        socket_,
        buffer_,
        request_,
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
        handle_error(error_code::map_boost_error_code(ec));
        return;
    }

    auto response = std::make_shared<response_type>(create_response());
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

void http_session::on_write(
    bool should_close, boost::system::error_code ec, std::size_t bytes_transferred
){
    if(ec){
        handle_error(error_code::map_boost_error_code(ec));
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

std::expected<void, error_code> http_session::close(){
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_send, ec);

    if(ec && ec != boost::asio::error::not_connected){
        return std::unexpected(error_code::map_boost_error_code(ec));
    }

    return {};
}

http_session::response_type http_session::create_response() const{
    response_type response{boost::beast::http::status::ok, request_.version()};

    return response;
}
