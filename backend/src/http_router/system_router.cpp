#include "http_router/system_router.hpp"

#include "http_router/route_table.hpp"

#include <array>

namespace{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        system_router::context_type,
        system_router::response_type
    >;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array health_pattern{
        path_segment_matcher::literal("health")
    };
    inline constexpr std::array supported_languages_pattern{
        path_segment_matcher::literal("supported-languages")
    };
}

system_router::response_type system_router::route(
    context_type& context,
    std::string_view path
){
    static const std::array<endpoint_descriptor, 2> system_route_table{{
        endpoint_descriptor{
            .name = "get_health",
            .method = http_verb::get,
            .operation_kind_value = http_route::operation_kind::query,
            .pattern = health_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return system_handler::get_health(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_supported_languages",
            .method = http_verb::get,
            .operation_kind_value = http_route::operation_kind::query,
            .pattern = supported_languages_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return system_handler::get_supported_languages(context_value);
            }
        }
    }};

    return http_route::dispatch_route_table(
        context,
        path,
        system_route_table
    );
}
