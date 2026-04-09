#include "http_router/auth_router.hpp"

#include "http_core/http_response_util.hpp"

auth_router::response_type auth_router::route(
    context_type& context,
    std::string_view path
){
    if(path == "/sign-up"){
        return handle_sign_up(context);
    }

    if(path == "/login"){
        return handle_login(context);
    }

    if(path == "/token/renew"){
        return handle_token_renew(context);
    }

    if(path == "/logout"){
        return handle_logout(context);
    }

    return http_response_util::create_not_found(context.request);
}

auth_router::response_type auth_router::handle_sign_up(context_type& context){
    if(context.request.method() == boost::beast::http::verb::post){
        return auth_handler::post_sign_up(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

auth_router::response_type auth_router::handle_login(context_type& context){
    if(context.request.method() == boost::beast::http::verb::post){
        return auth_handler::post_login(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

auth_router::response_type auth_router::handle_token_renew(context_type& context){
    if(context.request.method() == boost::beast::http::verb::post){
        return auth_handler::post_token_renew(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}

auth_router::response_type auth_router::handle_logout(context_type& context){
    if(context.request.method() == boost::beast::http::verb::post){
        return auth_handler::post_logout(context);
    }

    return http_response_util::create_method_not_allowed(context.request);
}
