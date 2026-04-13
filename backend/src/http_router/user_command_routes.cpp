#include "http_router/user_route_groups.hpp"

#include "http_handler/user_command_handler.hpp"

#include <array>

namespace{
    using endpoint_descriptor = user_route_groups::endpoint_descriptor;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array user_admin_pattern{
        path_segment_matcher::positive_int64("user_id"),
        path_segment_matcher::literal("admin")
    };
    inline constexpr std::array user_regular_pattern{
        path_segment_matcher::positive_int64("user_id"),
        path_segment_matcher::literal("user")
    };
    inline constexpr std::array user_submission_ban_pattern{
        path_segment_matcher::positive_int64("user_id"),
        path_segment_matcher::literal("submission-ban")
    };

    const auto& route_table(){
        static const std::array<endpoint_descriptor, 4> route_table_value{{
            endpoint_descriptor{
                .name = "put_user_admin",
                .method = http_verb::put,
                .kind = http_route::operation_kind::command,
                .pattern = user_admin_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_command_handler::put_user_admin(
                        context_value,
                        route_match_value.int64_param("user_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "put_user_regular",
                .method = http_verb::put,
                .kind = http_route::operation_kind::command,
                .pattern = user_regular_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_command_handler::put_user_regular(
                        context_value,
                        route_match_value.int64_param("user_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "post_user_submission_ban",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = user_submission_ban_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_command_handler::post_user_submission_ban(
                        context_value,
                        route_match_value.int64_param("user_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "delete_user_submission_ban",
                .method = http_verb::delete_,
                .kind = http_route::operation_kind::command,
                .pattern = user_submission_ban_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_command_handler::delete_user_submission_ban(
                        context_value,
                        route_match_value.int64_param("user_id")
                    );
                }
            }
        }};

        return route_table_value;
    }
}

std::span<const user_route_groups::endpoint_descriptor>
user_route_groups::command_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
