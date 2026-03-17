#include "http_router/system_router.hpp"

#include "http_server/http_util.hpp"

system_router::response_type system_router::route(
    const request_type& request,
    std::string_view path
){
    if(path == "/health"){
        return handle_health(request);
    }

    return http_util::not_found_response(request);
}

system_router::response_type system_router::handle_health(const request_type& request){
    if(request.method() == boost::beast::http::verb::get){
        return system_handler_.handle_health_get(request);
    }

    return http_util::method_not_allowed_response(request);
}
