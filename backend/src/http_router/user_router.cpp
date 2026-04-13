#include "http_router/user_router.hpp"

#include "http_router/user_route_groups.hpp"

#include <array>
#include <vector>

namespace{
    using endpoint_descriptor = user_route_groups::endpoint_descriptor;

    std::vector<endpoint_descriptor> make_user_route_table(){
        const auto query_routes = user_route_groups::query_routes();
        const auto command_routes = user_route_groups::command_routes();

        std::vector<endpoint_descriptor> route_table;
        route_table.reserve(17);

        route_table.insert(
            route_table.end(),
            query_routes.begin(),
            query_routes.begin() + 12
        );
        route_table.push_back(command_routes[0]);
        route_table.push_back(command_routes[1]);
        route_table.push_back(query_routes[12]);
        route_table.push_back(command_routes[2]);
        route_table.push_back(command_routes[3]);

        return route_table;
    }

    const auto& user_route_table(){
        static const auto route_table = make_user_route_table();
        return route_table;
    }
}

user_router::response_type user_router::route(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        std::span<const endpoint_descriptor>{user_route_table()}
    );
}
