#pragma once

#include "http_router/route_table.hpp"
#include "http_router/system_router.hpp"

#include <span>

namespace system_route_groups{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        system_router::context_type,
        system_router::response_type
    >;

    std::span<const endpoint_descriptor> public_routes();
    std::span<const endpoint_descriptor> db_routes();
}
