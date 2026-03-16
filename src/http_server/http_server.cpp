#include "http_server/http_server.hpp"

#include "db/db_connection.hpp"
#include "http_server/http_router.hpp"

#include <utility>

std::expected<std::shared_ptr<http_server>, error_code> http_server::create(){
    auto db_connection_exp = db_connection::create();
    if(!db_connection_exp){
        return std::unexpected(db_connection_exp.error());
    }

    auto http_router_exp = http_router::create(std::move(*db_connection_exp));
    if(!http_router_exp){
        return std::unexpected(http_router_exp.error());
    }

    return std::shared_ptr<http_server>(new http_server(std::move(*http_router_exp)));
}

http_server::http_server(http_router&& http_router) :
    http_router_(std::move(http_router)){}

http_server::response_type http_server::handle(const request_type& request){
    return http_router_.handle(request);
}
