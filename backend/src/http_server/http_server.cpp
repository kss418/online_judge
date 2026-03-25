#include "http_server/http_server.hpp"

#include "common/db_connection.hpp"
#include "http_server/http_dispatcher.hpp"

#include <utility>

std::expected<std::shared_ptr<http_server>, error_code> http_server::create(){
    auto db_connection_exp = db_connection::create();
    if(!db_connection_exp){
        return std::unexpected(db_connection_exp.error());
    }

    auto http_dispatcher_exp = http_dispatcher::create(std::move(*db_connection_exp));
    if(!http_dispatcher_exp){
        return std::unexpected(http_dispatcher_exp.error());
    }

    return std::shared_ptr<http_server>(new http_server(std::move(*http_dispatcher_exp)));
}

http_server::http_server(http_dispatcher&& http_dispatcher) :
    http_dispatcher_(std::move(http_dispatcher)){}

http_server::response_type http_server::handle(const request_type& request){
    return http_dispatcher_.handle(request);
}
