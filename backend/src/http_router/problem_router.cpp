#include "http_router/problem_router.hpp"

#include "http_handler/problem_command_handler.hpp"
#include "http_handler/problem_content_command_handler.hpp"
#include "http_handler/problem_content_query_handler.hpp"
#include "http_handler/problem_query_handler.hpp"
#include "http_handler/testcase_command_handler.hpp"
#include "http_handler/testcase_query_handler.hpp"
#include "http_router/route_table.hpp"

#include <array>

namespace{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        problem_router::context_type,
        problem_router::response_type
    >;
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
    inline constexpr std::array rejudge_pattern{
        path_segment_matcher::positive_int64("problem_id"),
        path_segment_matcher::literal("rejudge")
    };
}

problem_router::response_type problem_router::route(
    context_type& context,
    std::string_view path
){
    static const std::array<endpoint_descriptor, 21> problem_route_table{{
        endpoint_descriptor{
            .name = "get_problems",
            .method = http_verb::get,
            .operation_kind_value = http_route::operation_kind::query,
            .pattern = http_route::empty_path_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return problem_query_handler::get_problems(context_value);
            }
        },
        endpoint_descriptor{
            .name = "post_problem",
            .method = http_verb::post,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = http_route::empty_path_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return problem_command_handler::post_problem(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_problem",
            .method = http_verb::get,
            .operation_kind_value = http_route::operation_kind::query,
            .pattern = problem_id_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_query_handler::get_problem(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "delete_problem",
            .method = http_verb::delete_,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = problem_id_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_command_handler::delete_problem(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "put_problem_title",
            .method = http_verb::put,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = title_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_command_handler::put_problem(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_problem_limits",
            .method = http_verb::get,
            .operation_kind_value = http_route::operation_kind::query,
            .pattern = limits_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_content_query_handler::get_limits(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "put_problem_limits",
            .method = http_verb::put,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = limits_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_content_command_handler::put_limits(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "put_problem_statement",
            .method = http_verb::put,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = statement_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_content_command_handler::put_statement(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_problem_samples",
            .method = http_verb::get,
            .operation_kind_value = http_route::operation_kind::query,
            .pattern = samples_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_content_query_handler::get_samples(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "post_problem_sample",
            .method = http_verb::post,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = samples_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_content_command_handler::post_sample(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "delete_problem_sample",
            .method = http_verb::delete_,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = samples_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_content_command_handler::delete_sample(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "put_problem_sample",
            .method = http_verb::put,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = sample_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_content_command_handler::put_sample(
                    context_value,
                    route_match_value.int64_param("problem_id"),
                    route_match_value.int32_param("sample_order")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_problem_testcases",
            .method = http_verb::get,
            .operation_kind_value = http_route::operation_kind::query,
            .pattern = testcases_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return testcase_query_handler::get_testcases(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "post_problem_testcase",
            .method = http_verb::post,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = testcases_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return testcase_command_handler::post_testcase(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "delete_problem_testcase_all",
            .method = http_verb::delete_,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = testcase_all_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return testcase_command_handler::delete_all_testcases(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "post_problem_testcase_zip",
            .method = http_verb::post,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = testcase_zip_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return testcase_command_handler::post_testcase_zip(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "post_problem_testcase_move",
            .method = http_verb::post,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = testcase_move_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return testcase_command_handler::move_testcase(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_problem_testcase",
            .method = http_verb::get,
            .operation_kind_value = http_route::operation_kind::query,
            .pattern = testcase_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
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
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = testcase_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
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
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = testcase_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return testcase_command_handler::delete_testcase(
                    context_value,
                    route_match_value.int64_param("problem_id"),
                    route_match_value.int32_param("testcase_order")
                );
            }
        },
        endpoint_descriptor{
            .name = "post_problem_rejudge",
            .method = http_verb::post,
            .operation_kind_value = http_route::operation_kind::command,
            .pattern = rejudge_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return problem_command_handler::post_problem_rejudge(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        }
    }};

    return http_route::dispatch_route_table(
        context,
        path,
        problem_route_table
    );
}
