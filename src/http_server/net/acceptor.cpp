#include "http_server/net/acceptor.hpp"
#include "http_server/net/http_session.hpp"

#include <boost/asio/strand.hpp>
#include <iostream>
#include <memory>
#include <utility>

std::expected<std::shared_ptr<acceptor>, error_code> acceptor::create(
    boost::asio::io_context& io_context, const tcp::endpoint& endpoint
){
    auto created_acceptor = std::shared_ptr<acceptor>(new acceptor(io_context));
    auto initialize_exp = created_acceptor->initialize(endpoint);
    if(!initialize_exp){
        return std::unexpected(initialize_exp.error());
    }

    return created_acceptor;
}

acceptor::acceptor(boost::asio::io_context& io_context) :
    io_context_(io_context),
    acceptor_(boost::asio::make_strand(io_context_)){}

std::expected<void, error_code> acceptor::initialize(const tcp::endpoint& endpoint){
    boost::system::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if(ec){
        return std::unexpected(error_code::map_boost_error_code(ec));
    }

    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if(ec){
        return std::unexpected(error_code::map_boost_error_code(ec));
    }

    acceptor_.bind(endpoint, ec);
    if(ec){
        return std::unexpected(error_code::map_boost_error_code(ec));
    }

    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if(ec){
        return std::unexpected(error_code::map_boost_error_code(ec));
    }

    return {};
}

std::expected<void, error_code> acceptor::run(){
    if(!acceptor_.is_open()){
        return std::unexpected(error_code::create(boost_error::bad_descriptor));
    }

    accept();
    return {};
}

void acceptor::handle_error(error_code code) const{
    std::cerr << "acceptor error: " << to_string(code) << '\n';
}

void acceptor::accept(){
    auto self = shared_from_this();
    acceptor_.async_accept(
        boost::asio::make_strand(io_context_),
        [self](boost::system::error_code ec, tcp::socket socket){
            self->on_accept(ec, std::move(socket));
        }
    );
}

void acceptor::on_accept(boost::system::error_code ec, tcp::socket socket){
    if(ec){
        handle_error(error_code::map_boost_error_code(ec));
    } 
    else{
        auto session_exp = http_session::create(std::move(socket));
        if(!session_exp){
            handle_error(session_exp.error());
        } 
        else{
            auto run_exp = (*session_exp)->run();
            if(!run_exp){
                handle_error(run_exp.error());
            }
        }
    }

    accept();
}
