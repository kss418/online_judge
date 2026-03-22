#include "http_handler/system_handler.hpp"
#include "http_server/json_util.hpp"
#include "http_server/http_response_util.hpp"

system_handler::response_type system_handler::handle_health_get(const request_type& request){
    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_message_object("ok")
    );
}
