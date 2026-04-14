#include "http_router/submission_route_groups.hpp"

#include "http_handler/submission_command_handler.hpp"

#include <array>

namespace{
    using endpoint_descriptor = submission_route_groups::endpoint_descriptor;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array problem_submission_pattern{
        path_segment_matcher::positive_int64("problem_id")
    };
    inline constexpr std::array submission_rejudge_pattern{
        path_segment_matcher::positive_int64("submission_id"),
        path_segment_matcher::literal("rejudge")
    };

    const auto& route_table(){
        static const std::array<endpoint_descriptor, 2> route_table_value{{
            endpoint_descriptor{
                .name = "post_submission",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = problem_submission_pattern,
                .invoke = [](submission_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> submission_router::response_type {
                    return submission_command_handler::post_submission(
                        context_value,
                        route_match_value.int64_param("problem_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "post_submission_rejudge",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = submission_rejudge_pattern,
                .invoke = [](submission_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> submission_router::response_type {
                    return submission_command_handler::post_submission_rejudge(
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
submission_route_groups::command_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
