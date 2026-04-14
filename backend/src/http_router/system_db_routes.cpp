#include "http_router/system_route_groups.hpp"

#include "http_handler/system_query_handler.hpp"

#include <array>

namespace{
    using endpoint_descriptor = system_route_groups::endpoint_descriptor;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array status_pattern{
        path_segment_matcher::literal("status")
    };

    const auto& route_table(){
        static const std::array<endpoint_descriptor, 1> route_table_value{{
            endpoint_descriptor{
                .name = "get_status",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = status_pattern,
                .invoke = [](system_router::context_type& context_value,
                    const http_route::route_match&)
                    -> system_router::response_type {
                    return system_query_handler::get_status(context_value);
                }
            }
        }};

        return route_table_value;
    }
}

std::span<const system_route_groups::endpoint_descriptor>
system_route_groups::db_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
