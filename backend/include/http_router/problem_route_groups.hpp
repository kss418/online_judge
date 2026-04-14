#pragma once

#include "http_router/problem_router.hpp"
#include "http_router/route_table.hpp"

#include <span>

namespace problem_route_groups{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        problem_router::context_type,
        problem_router::response_type
    >;

    std::span<const endpoint_descriptor> query_routes();
    std::span<const endpoint_descriptor> command_routes();
    std::span<const endpoint_descriptor> testcase_routes();
    std::span<const endpoint_descriptor> all_routes();
}
