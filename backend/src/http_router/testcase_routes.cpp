#include "http_router/problem_route_groups.hpp"

#include "http_handler/testcase_command_handler.hpp"
#include "http_handler/testcase_query_handler.hpp"

#include <array>

namespace{
    using endpoint_descriptor = problem_route_groups::endpoint_descriptor;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array testcases_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("testcase")
    };
    inline constexpr std::array testcase_all_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("testcase"),
        path_segment_matcher::literal("all")
    };
    inline constexpr std::array testcase_zip_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("testcase"),
        path_segment_matcher::literal("zip")
    };
    inline constexpr std::array testcase_move_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("testcase"),
        path_segment_matcher::literal("move")
    };
    inline constexpr std::array testcase_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("testcase"),
        path_segment_matcher::positive_int32("testcase_order")
    };

    const auto& route_table(){
        static const std::array<endpoint_descriptor, 8> route_table_value{{
            endpoint_descriptor{
                .name = "get_problem_testcases",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = testcases_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return testcase_query_handler::get_testcases(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "post_problem_testcase",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = testcases_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return testcase_command_handler::post_testcase(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "delete_problem_testcase_all",
                .method = http_verb::delete_,
                .kind = http_route::operation_kind::command,
                .pattern = testcase_all_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return testcase_command_handler::delete_all_testcases(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "post_problem_testcase_zip",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = testcase_zip_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return testcase_command_handler::post_testcase_zip(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "post_problem_testcase_move",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = testcase_move_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return testcase_command_handler::move_testcase(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_problem_testcase",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = testcase_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return testcase_query_handler::get_testcase(
                        context_value,
                        route_match_value.int64_param("problem_id"),
                        route_match_value.int32_param("testcase_order")
                    );
                }
            },
            endpoint_descriptor{
                .name = "put_problem_testcase",
                .method = http_verb::put,
                .kind = http_route::operation_kind::command,
                .pattern = testcase_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return testcase_command_handler::put_testcase(
                        context_value,
                        route_match_value.int64_param("problem_id"),
                        route_match_value.int32_param("testcase_order")
                    );
                }
            },
            endpoint_descriptor{
                .name = "delete_problem_testcase",
                .method = http_verb::delete_,
                .kind = http_route::operation_kind::command,
                .pattern = testcase_pattern,
                .invoke = [](problem_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> problem_router::response_type {
                    return testcase_command_handler::delete_testcase(
                        context_value,
                        route_match_value.int64_param("problem_id"),
                        route_match_value.int32_param("testcase_order")
                    );
                }
            }
        }};

        return route_table_value;
    }
}

std::span<const problem_route_groups::endpoint_descriptor>
problem_route_groups::testcase_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
