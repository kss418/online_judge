#include "http_handler/system_handler.hpp"
#include "http_server/http_util.hpp"

system_handler::response_type system_handler::handle_health_get(const request_type& request){
    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "ok\n"
    );
}
