#include "http_server/http_server.hpp"

#include "db/db_connection.hpp"
#include "db/submission_service.hpp"
#include "http_server/acceptor.hpp"
#include "http_server/http_handler.hpp"

#include <utility>

std::expected<std::shared_ptr<http_server>, error_code> http_server::create(
    boost::asio::io_context& io_context,
    const tcp::endpoint& endpoint
){
    auto db_connection_exp = db_connection::create();
    if(!db_connection_exp){
        return std::unexpected(db_connection_exp.error());
    }

    auto submission_service_exp = submission_service::create(std::move(*db_connection_exp));
    if(!submission_service_exp){
        return std::unexpected(submission_service_exp.error());
    }

    auto handler_exp = http_handler::create(std::move(*submission_service_exp));
    if(!handler_exp){
        return std::unexpected(handler_exp.error());
    }

    auto created_server = std::shared_ptr<http_server>(
        new http_server(io_context, std::move(*handler_exp))
    );
    auto initialize_exp = created_server->initialize(endpoint);
    if(!initialize_exp){
        return std::unexpected(initialize_exp.error());
    }

    return created_server;
}

http_server::http_server(
    boost::asio::io_context& io_context,
    http_handler http_handler
) :
    io_context_(io_context),
    http_handler_(std::move(http_handler)){}

std::expected<void, error_code> http_server::initialize(const tcp::endpoint& endpoint){
    auto acceptor_exp = acceptor::create(io_context_, endpoint, shared_from_this());
    if(!acceptor_exp){
        return std::unexpected(acceptor_exp.error());
    }

    acceptor_ = std::move(*acceptor_exp);
    return {};
}

std::expected<void, error_code> http_server::run(){
    if(!acceptor_){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return acceptor_->run();
}

http_server::response_type http_server::handle(const request_type& request){
    return http_handler_.handle(request);
}
