#include "http_handler/system_handler.hpp"
#include "http_server/http_util.hpp"

bool system_handler::is_system_path(std::string_view path){
    return path.starts_with(path_prefix_);
}

system_handler::response_type system_handler::handle(
    const request_type& request,
    std::string_view path
){
    if(path == "/health"){
        if(request.method() == boost::beast::http::verb::get){
            return handle_health_get(request);
        }

        return http_util::create_text_response(
            request,
            boost::beast::http::status::method_not_allowed,
            "method not allowed\n"
        );
    }

    return http_util::create_text_response(
        request,
        boost::beast::http::status::not_found,
        "not found\n"
    );
}

system_handler::response_type system_handler::handle_health_get(const request_type& request){
    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "ok\n"
    );
}
