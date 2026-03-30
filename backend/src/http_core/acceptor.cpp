#include "http_core/acceptor.hpp"
#include "common/logger.hpp"
#include "http_core/http_server.hpp"
#include "http_core/http_session.hpp"

#include <boost/asio/strand.hpp>
#include <memory>
#include <utility>

std::expected<std::shared_ptr<acceptor>, error_code> acceptor::create(
    boost::asio::io_context& io_context,
    const tcp::endpoint& endpoint,
    std::shared_ptr<http_server> http_server
){
    if(!http_server){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    auto acceptor_ptr = std::shared_ptr<acceptor>(
        new acceptor(io_context, std::move(http_server))
    );

    auto initialize_exp = acceptor_ptr->initialize(endpoint);
    if(!initialize_exp){
        return std::unexpected(initialize_exp.error());
    }

    return acceptor_ptr;
}

acceptor::acceptor(boost::asio::io_context& io_context, std::shared_ptr<http_server> http_server) :
    io_context_(io_context),
    acceptor_(boost::asio::make_strand(io_context_)),
    http_server_(std::move(http_server)){}

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
    
    if(!http_server_){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    accept();
    return {};
}

void acceptor::handle_error(error_code code) const{
    logger::cerr()
        .log("acceptor_error")
        .field("code", to_string(code));
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
        if(ec == boost::asio::error::operation_aborted){
            return;
        }

        handle_error(error_code::map_boost_error_code(ec));
    } 
    else{
        if(!http_server_){
            handle_error(error_code::create(errno_error::invalid_argument));
        }
        else{
            auto session_exp = http_session::create(std::move(socket), http_server_);
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
    }

    if(!acceptor_.is_open()){
        return;
    }

    accept();
}
