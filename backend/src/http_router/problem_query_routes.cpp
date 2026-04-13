#include "http_router/problem_route_groups.hpp"

#include "http_handler/problem_content_query_handler.hpp"
#include "http_handler/problem_query_handler.hpp"

#include <array>

namespace{
    using endpoint_descriptor = problem_route_groups::endpoint_descriptor;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array problem_id_pattern{
        path_segment_matcher::positive_int64("problem_id")
    };
    inline constexpr std::array limits_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("limits")
    };
    inline constexpr std::array samples_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("sample")
    };

    const auto& route_table(){
        static const std::array<endpoint_descriptor, 4> route_table_value{{
            endpoint_descriptor{
                .name = "get_problems",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = http_route::empty_path_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match&) -> problem_router::response_type {
                    return problem_query_handler::get_problems(context_value);
                }
            },
            endpoint_descriptor{
                .name = "get_problem",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = problem_id_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_query_handler::get_problem(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_problem_limits",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = limits_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_content_query_handler::get_limits(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_problem_samples",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = samples_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_content_query_handler::get_samples(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            }
        }};

        return route_table_value;
    }
}

std::span<const problem_route_groups::endpoint_descriptor>
problem_route_groups::query_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
