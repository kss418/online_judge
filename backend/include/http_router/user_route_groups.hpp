#pragma once

#include "http_router/route_table.hpp"
#include "http_router/user_router.hpp"

#include <span>

namespace user_route_groups{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        user_router::context_type,
        user_router::response_type
    >;

    std::span<const endpoint_descriptor> query_routes();
    std::span<const endpoint_descriptor> command_routes();
    std::span<const endpoint_descriptor> all_routes();
}
