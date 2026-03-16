#include "http_server/http_router.hpp"
#include "http_server/http_util.hpp"

#include <functional>
#include <string_view>
#include <utility>

std::expected<http_router, error_code> http_router::create(db_connection db_connection){
    auto http_handler_exp = http_handler::create(std::move(db_connection));
    if(!http_handler_exp){
        return std::unexpected(http_handler_exp.error());
    }

    return http_router(std::move(*http_handler_exp));
}

http_router::http_router(http_handler&& http_handler) :
    http_handler_(std::move(http_handler)){}

std::optional<http_router::route_handler> http_router::find_route_handler(
    boost::beast::http::verb method,
    std::string_view path
){
    for(const auto& route_definition_value : routes_){
        if(route_definition_value.method == method && route_definition_value.path == path){
            return route_definition_value.handler;
        }
    }

    return std::nullopt;
}

bool http_router::has_route_path(std::string_view path){
    for(const auto& route_definition_value : routes_){
        if(route_definition_value.path == path){
            return true;
        }
    }

    return false;
}

http_router::response_type http_router::handle(const request_type& request){
    const std::string_view path{
        request.target().data(),
        request.target().size()
    };

    const auto route_handler_opt = find_route_handler(request.method(), path);
    if(route_handler_opt.has_value()){
        return std::invoke(route_handler_opt.value(), http_handler_, request);
    }

    if(has_route_path(path)){
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
