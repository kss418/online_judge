#include "http_server/http_handler.hpp"
#include "http_server/http_util.hpp"

#include <utility>

std::expected<http_handler, error_code> http_handler::create(
    db_connection db_connection
){
    if(!db_connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return http_handler(std::move(db_connection));
}

http_handler::http_handler(db_connection db_connection) :
    db_connection_(std::move(db_connection)){}

http_handler::response_type http_handler::handle_health_get(const request_type& request){
    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "ok\n"
    );
}
