#pragma once

#include "http_router/auth_router.hpp"
#include "http_router/route_table.hpp"

#include <span>

namespace auth_route_groups{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        auth_router::context_type,
        auth_router::response_type
    >;

    std::span<const endpoint_descriptor> command_routes();
    std::span<const endpoint_descriptor> all_routes();
}
