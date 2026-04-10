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
    inline constexpr std::array status_pattern{
        path_segment_matcher::literal("status")
    };

    endpoint_descriptor make_get_health_endpoint(){
        return endpoint_descriptor{
            .name = "get_health",
            .method = http_verb::get,
            .kind = http_route::operation_kind::query,
            .pattern = health_pattern,
            .invoke = [](system_router::context_type& context_value,
                const http_route::route_match&) -> system_router::response_type {
                return system_query_handler::get_health(context_value);
            }
        };
    }

    endpoint_descriptor make_get_supported_languages_endpoint(){
        return endpoint_descriptor{
            .name = "get_supported_languages",
            .method = http_verb::get,
            .kind = http_route::operation_kind::query,
            .pattern = supported_languages_pattern,
            .invoke = [](system_router::context_type& context_value,
                const http_route::route_match&) -> system_router::response_type {
                return system_query_handler::get_supported_languages(context_value);
            }
        };
    }

    endpoint_descriptor make_get_status_endpoint(){
        return endpoint_descriptor{
            .name = "get_status",
            .method = http_verb::get,
            .kind = http_route::operation_kind::query,
            .pattern = status_pattern,
            .invoke = [](system_router::context_type& context_value,
                const http_route::route_match&) -> system_router::response_type {
                return system_query_handler::get_status(context_value);
            }
        };
    }

    const auto& public_system_route_table(){
        static const std::array<endpoint_descriptor, 2> route_table{{
            make_get_health_endpoint(),
            make_get_supported_languages_endpoint()
        }};
        return route_table;
    }

    const auto& system_route_table(){
        static const std::array<endpoint_descriptor, 3> route_table{{
            make_get_health_endpoint(),
            make_get_supported_languages_endpoint(),
            make_get_status_endpoint()
        }};
        return route_table;
    }
}

system_router::response_type system_router::route_public(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        public_system_route_table()
    );
}

system_router::response_type system_router::route(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        system_route_table()
    );
}
