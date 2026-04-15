#include "http_router/submission_router.hpp"

#include "http_router/submission_route_groups.hpp"

namespace{
    using endpoint_descriptor = submission_route_groups::endpoint_descriptor;

    const auto& route_table(){
        static const auto route_table_value = http_route::concat_routes<endpoint_descriptor>(
            submission_route_groups::query_routes(),
            submission_route_groups::command_routes()
        );
        return route_table_value;
    }
}

submission_router::response_type submission_router::route(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        std::span<const endpoint_descriptor>{route_table()}
    );
}
