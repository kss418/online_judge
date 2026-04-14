#include "http_router/problem_router.hpp"

#include "http_router/problem_route_groups.hpp"

problem_router::response_type problem_router::route(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        problem_route_groups::all_routes()
    );
}
