#include "http_router/submission_router.hpp"

#include "http_router/submission_route_groups.hpp"

submission_router::response_type submission_router::route(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        submission_route_groups::all_routes()
    );
}
