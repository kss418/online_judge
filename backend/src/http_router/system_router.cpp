#include "http_router/system_router.hpp"

#include "http_core/http_response_util.hpp"

system_router::response_type system_router::route(
    const request_type& request,
    std::string_view path
){
    if(path == "/health"){
        return handle_health(request);
    }

    if(path == "/supported-languages"){
        return handle_supported_languages(request);
    }

    return http_response_util::create_not_found(request);
}

system_router::response_type system_router::handle_health(const request_type& request){
    if(request.method() == boost::beast::http::verb::get){
        return system_handler_.handle_health_get(request);
    }

    return http_response_util::create_method_not_allowed(request);
}

system_router::response_type system_router::handle_supported_languages(
    const request_type& request
){
    if(request.method() == boost::beast::http::verb::get){
        return system_handler_.handle_supported_languages_get(request);
    }

    return http_response_util::create_method_not_allowed(request);
}
