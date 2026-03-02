#include "http_server/http_server.hpp"

#include "db/db_connection.hpp"
#include "db/submission_service.hpp"
#include "http_server/http_handler.hpp"

#include <utility>

std::expected<std::shared_ptr<http_server>, error_code> http_server::create(){
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

    return std::shared_ptr<http_server>(new http_server(std::move(*handler_exp)));
}

http_server::http_server(http_handler http_handler) :
    http_handler_(std::move(http_handler)){}

http_server::response_type http_server::handle(const request_type& request){
    return http_handler_.handle(request);
}
