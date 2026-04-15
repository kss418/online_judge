#include "http_router/auth_router.hpp"

#include "http_router/auth_route_groups.hpp"

auth_router::response_type auth_router::route(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        auth_route_groups::command_routes()
    );
}
