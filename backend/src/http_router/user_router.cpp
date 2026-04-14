#include "http_router/user_router.hpp"

#include "http_router/user_route_groups.hpp"

user_router::response_type user_router::route(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        user_route_groups::all_routes()
    );
}
