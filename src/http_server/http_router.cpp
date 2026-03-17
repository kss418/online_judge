#include "http_server/http_router.hpp"
#include "http_server/http_util.hpp"

#include <functional>
#include <string_view>
#include <utility>

std::expected<http_router, error_code> http_router::create(db_connection db_connection){
    if(!db_connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return http_router(std::move(db_connection));
}

http_router::http_router(db_connection db_connection) :
    db_connection_(std::move(db_connection)),
    system_handler_(),
    auth_handler_(db_connection_){}

http_router::http_router(http_router&& other) noexcept :
    db_connection_(std::move(other.db_connection_)),
    system_handler_(),
    auth_handler_(db_connection_){}

std::optional<http_router::route_handler<system_handler>> http_router::find_system_route_handler(
    boost::beast::http::verb method,
    std::string_view path
){
    for(const auto& route_definition_value : system_routes_){
        if(route_definition_value.method == method && route_definition_value.path == path){
            return route_definition_value.handler;
        }
    }

    return std::nullopt;
}

std::optional<http_router::route_handler<auth_handler>> http_router::find_auth_route_handler(
    boost::beast::http::verb method,
    std::string_view path
){
    for(const auto& route_definition_value : auth_routes_){
        if(route_definition_value.method == method && route_definition_value.path == path){
            return route_definition_value.handler;
        }
    }

    return std::nullopt;
}

std::optional<http_router::response_type> http_router::try_handle_route(
    const request_type& request
){
    const std::string_view path{
        request.target().data(),
        request.target().size()
    };

    if(system_handler::is_system_path(path)){
        const auto system_route_handler_opt = find_system_route_handler(request.method(), path);
        if(system_route_handler_opt.has_value()){
            return std::invoke(system_route_handler_opt.value(), system_handler_, request);
        }
    }

    if(auth_handler::is_auth_path(path)){
        const auto auth_route_handler_opt = find_auth_route_handler(request.method(), path);
        if(auth_route_handler_opt.has_value()){
            return std::invoke(auth_route_handler_opt.value(), auth_handler_, request);
        }
    }

    return std::nullopt;
}

bool http_router::has_route_path(std::string_view path){
    for(const auto& route_definition_value : system_routes_){
        if(route_definition_value.path == path){
            return true;
        }
    }

    for(const auto& route_definition_value : auth_routes_){
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

    const auto response_opt = try_handle_route(request);
    if(response_opt.has_value()){
        return std::move(response_opt.value());
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
