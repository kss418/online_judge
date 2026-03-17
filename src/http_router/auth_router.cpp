#include "http_router/auth_router.hpp"

#include "http_server/http_util.hpp"

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

    return http_util::not_found_response(request);
}

auth_router::response_type auth_router::handle_sign_up(const request_type& request){
    if(request.method() == boost::beast::http::verb::post){
        return auth_handler::handle_sign_up_post(request, db_connection_);
    }

    return http_util::method_not_allowed_response(request);
}

auth_router::response_type auth_router::handle_login(const request_type& request){
    if(request.method() == boost::beast::http::verb::post){
        return auth_handler::handle_login_post(request, db_connection_);
    }

    return http_util::method_not_allowed_response(request);
}

auth_router::response_type auth_router::handle_token_renew(const request_type& request){
    if(request.method() == boost::beast::http::verb::post){
        return auth_handler::handle_token_renew_post(request, db_connection_);
    }

    return http_util::method_not_allowed_response(request);
}

auth_router::response_type auth_router::handle_logout(const request_type& request){
    if(request.method() == boost::beast::http::verb::post){
        return auth_handler::handle_logout_post(request, db_connection_);
    }

    return http_util::method_not_allowed_response(request);
}
