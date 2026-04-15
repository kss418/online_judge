#pragma once

#include "http_router/route_table.hpp"
#include "http_router/submission_router.hpp"

#include <span>

namespace submission_route_groups{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        submission_router::context_type,
        submission_router::response_type
    >;

    std::span<const endpoint_descriptor> query_routes();
    std::span<const endpoint_descriptor> command_routes();
}
