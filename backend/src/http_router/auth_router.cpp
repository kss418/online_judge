#include "http_router/auth_router.hpp"

#include "http_core/http_response_util.hpp"

auth_router::auth_router(db_connection& db_connection) :
    db_connection_(db_connection){}

auth_router::response_type auth_router::route(
    const request_type& request,
    std::string_view path
){
    if(path == "/sign-up"){
        return handle_sign_up(request);
    }

    if(path == "/login"){
        return handle_login(request);
    }

    if(path == "/token/renew"){
        return handle_token_renew(request);
    }

    if(path == "/logout"){
        return handle_logout(request);
    }

    return http_response_util::create_not_found(request);
}

auth_router::response_type auth_router::handle_sign_up(const request_type& request){
    if(request.method() == boost::beast::http::verb::post){
        return auth_handler::post_sign_up(request, db_connection_);
    }

    return http_response_util::create_method_not_allowed(request);
}

auth_router::response_type auth_router::handle_login(const request_type& request){
    if(request.method() == boost::beast::http::verb::post){
        return auth_handler::post_login(request, db_connection_);
    }

    return http_response_util::create_method_not_allowed(request);
}

auth_router::response_type auth_router::handle_token_renew(const request_type& request){
    if(request.method() == boost::beast::http::verb::post){
        return auth_handler::post_token_renew(request, db_connection_);
    }

    return http_response_util::create_method_not_allowed(request);
}

auth_router::response_type auth_router::handle_logout(const request_type& request){
    if(request.method() == boost::beast::http::verb::post){
        return auth_handler::post_logout(request, db_connection_);
    }

    return http_response_util::create_method_not_allowed(request);
}
