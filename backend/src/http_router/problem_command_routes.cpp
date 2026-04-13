#include "http_router/problem_route_groups.hpp"

#include "http_handler/problem_command_handler.hpp"
#include "http_handler/problem_content_command_handler.hpp"

#include <array>

namespace{
    using endpoint_descriptor = problem_route_groups::endpoint_descriptor;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array problem_id_pattern{
        path_segment_matcher::positive_int64("problem_id")
    };
    inline constexpr std::array title_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("title")
    };
    inline constexpr std::array limits_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("limits")
    };
    inline constexpr std::array statement_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("statement")
    };
    inline constexpr std::array samples_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("sample")
    };
    inline constexpr std::array sample_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("sample"),
        path_segment_matcher::positive_int32("sample_order")
    };
    inline constexpr std::array rejudge_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("rejudge")
    };

    const auto& route_table(){
        static const std::array<endpoint_descriptor, 9> route_table_value{{
            endpoint_descriptor{
                .name = "post_problem",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = http_route::empty_path_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match&) -> problem_router::response_type {
                    return problem_command_handler::post_problem(context_value);
                }
            },
            endpoint_descriptor{
                .name = "delete_problem",
                .method = http_verb::delete_,
                .kind = http_route::operation_kind::command,
                .pattern = problem_id_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_command_handler::delete_problem(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "put_problem_title",
                .method = http_verb::put,
                .kind = http_route::operation_kind::command,
                .pattern = title_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_command_handler::put_problem(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "put_problem_limits",
                .method = http_verb::put,
                .kind = http_route::operation_kind::command,
                .pattern = limits_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_content_command_handler::put_limits(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "put_problem_statement",
                .method = http_verb::put,
                .kind = http_route::operation_kind::command,
                .pattern = statement_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_content_command_handler::put_statement(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "post_problem_sample",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = samples_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_content_command_handler::post_sample(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "delete_problem_sample",
                .method = http_verb::delete_,
                .kind = http_route::operation_kind::command,
                .pattern = samples_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_content_command_handler::delete_sample(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "put_problem_sample",
                .method = http_verb::put,
                .kind = http_route::operation_kind::command,
                .pattern = sample_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_content_command_handler::put_sample(
                        context_value,
                        route_match_value.int64_param("problem_id"),
                        route_match_value.int32_param("sample_order")
                    );
                }
            },
            endpoint_descriptor{
                .name = "post_problem_rejudge",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = rejudge_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return problem_command_handler::post_problem_rejudge(
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
problem_route_groups::command_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
