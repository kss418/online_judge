#include "http_router/submission_router.hpp"

#include "http_handler/submission_command_handler.hpp"
#include "http_handler/submission_query_handler.hpp"
#include "http_router/route_table.hpp"

#include <array>

namespace{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        submission_router::context_type,
        submission_router::response_type
    >;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array status_batch_pattern{
        path_segment_matcher::literal("status"),
        path_segment_matcher::literal("batch")
    };
    inline constexpr std::array submission_detail_pattern{
        path_segment_matcher::positive_int64("submission_id")
    };
    inline constexpr std::array problem_submission_pattern{
        path_segment_matcher::positive_int64("problem_id")
    };
    inline constexpr std::array submission_history_pattern{
        path_segment_matcher::positive_int64("submission_id"),
        path_segment_matcher::literal("history")
    };
    inline constexpr std::array submission_source_pattern{
        path_segment_matcher::positive_int64("submission_id"),
        path_segment_matcher::literal("source")
    };
    inline constexpr std::array submission_rejudge_pattern{
        path_segment_matcher::positive_int64("submission_id"),
        path_segment_matcher::literal("rejudge")
    };
}

submission_router::response_type submission_router::route(
    context_type& context,
    std::string_view path
){
    static const std::array<endpoint_descriptor, 7> submission_route_table{{
        endpoint_descriptor{
            .name = "get_submissions",
            .method = http_verb::get,
            .pattern = http_route::empty_path_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return submission_query_handler::get_submissions(context_value);
            }
        },
        endpoint_descriptor{
            .name = "post_submission_status_batch",
            .method = http_verb::post,
            .pattern = status_batch_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return submission_query_handler::post_submission_status_batch(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_submission",
            .method = http_verb::get,
            .pattern = submission_detail_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return submission_query_handler::get_submission(
                    context_value,
                    route_match_value.int64_param("submission_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "post_submission",
            .method = http_verb::post,
            .pattern = problem_submission_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return submission_command_handler::post_submission(
                    context_value,
                    route_match_value.int64_param("problem_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_submission_history",
            .method = http_verb::get,
            .pattern = submission_history_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return submission_query_handler::get_submission_history(
                    context_value,
                    route_match_value.int64_param("submission_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_submission_source",
            .method = http_verb::get,
            .pattern = submission_source_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return submission_query_handler::get_submission_source(
                    context_value,
                    route_match_value.int64_param("submission_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "post_submission_rejudge",
            .method = http_verb::post,
            .pattern = submission_rejudge_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return submission_command_handler::post_submission_rejudge(
                    context_value,
                    route_match_value.int64_param("submission_id")
                );
            }
        }
    }};

    return http_route::dispatch_route_table(
        context,
        path,
        submission_route_table
    );
}
