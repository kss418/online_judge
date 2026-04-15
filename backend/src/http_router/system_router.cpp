#include "http_router/system_router.hpp"

#include "http_router/system_route_groups.hpp"

system_router::response_type system_router::route_public(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        system_route_groups::public_routes()
    );
}

system_router::response_type system_router::route(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        system_route_groups::db_routes()
    );
}
