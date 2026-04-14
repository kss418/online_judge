#include "http_router/submission_route_groups.hpp"

#include "http_handler/submission_query_handler.hpp"

#include <array>

namespace{
    using endpoint_descriptor = submission_route_groups::endpoint_descriptor;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array status_batch_pattern{
        path_segment_matcher::literal("status"),
        path_segment_matcher::literal("batch")
    };
    inline constexpr std::array submission_detail_pattern{
        path_segment_matcher::positive_int64("submission_id")
    };
    inline constexpr std::array submission_history_pattern{
        path_segment_matcher::positive_int64("submission_id"),
        path_segment_matcher::literal("history")
    };
    inline constexpr std::array submission_source_pattern{
        path_segment_matcher::positive_int64("submission_id"),
        path_segment_matcher::literal("source")
    };

    const auto& route_table(){
        static const std::array<endpoint_descriptor, 5> route_table_value{{
            endpoint_descriptor{
                .name = "get_submissions",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = http_route::empty_path_pattern,
                .invoke = [](submission_router::context_type& context_value,
                    const http_route::route_match&)
                    -> submission_router::response_type {
                    return submission_query_handler::get_submissions(context_value);
                }
            },
            endpoint_descriptor{
                .name = "post_submission_status_batch",
                .method = http_verb::post,
                .kind = http_route::operation_kind::query,
                .pattern = status_batch_pattern,
                .invoke = [](submission_router::context_type& context_value,
                    const http_route::route_match&)
                    -> submission_router::response_type {
                    return submission_query_handler::post_submission_status_batch(
                        context_value
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_submission",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = submission_detail_pattern,
                .invoke = [](submission_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> submission_router::response_type {
                    return submission_query_handler::get_submission(
                        context_value,
                        route_match_value.int64_param("submission_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_submission_history",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = submission_history_pattern,
                .invoke = [](submission_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> submission_router::response_type {
                    return submission_query_handler::get_submission_history(
                        context_value,
                        route_match_value.int64_param("submission_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_submission_source",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = submission_source_pattern,
                .invoke = [](submission_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> submission_router::response_type {
                    return submission_query_handler::get_submission_source(
                        context_value,
                        route_match_value.int64_param("submission_id")
                    );
                }
            }
        }};

        return route_table_value;
    }
}

std::span<const submission_route_groups::endpoint_descriptor>
submission_route_groups::query_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
