#include "http_router/system_router.hpp"

#include "http_core/http_response_util.hpp"

system_router::response_type system_router::route(
    context_type& context,
    std::string_view path
){
    if(path == "/health"){
        return handle_health(context);
    }

    if(path == "/supported-languages"){
        return handle_supported_languages(context);
    }

    return http_response_util::create_not_found(context.request);
}

system_router::response_type system_router::handle_health(context_type& context){
    if(context.request.method() == boost::beast::http::verb::get){
        return system_handler_.handle_health_get(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

system_router::response_type system_router::handle_supported_languages(
    context_type& context
){
    if(context.request.method() == boost::beast::http::verb::get){
        return system_handler_.handle_supported_languages_get(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}
