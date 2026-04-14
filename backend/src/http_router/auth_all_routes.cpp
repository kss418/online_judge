#include "http_router/auth_route_groups.hpp"

namespace{
    using endpoint_descriptor = auth_route_groups::endpoint_descriptor;

    const auto& route_table(){
        static const auto route_table_value = http_route::concat_routes<endpoint_descriptor>(
            auth_route_groups::command_routes()
        );
        return route_table_value;
    }
}

std::span<const auth_route_groups::endpoint_descriptor>
auth_route_groups::all_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
