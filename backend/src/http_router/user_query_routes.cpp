#include "http_router/user_route_groups.hpp"

#include "http_handler/user_list_query_handler.hpp"
#include "http_handler/user_problem_list_query_handler.hpp"
#include "http_handler/user_profile_query_handler.hpp"
#include "http_handler/user_statistics_query_handler.hpp"
#include "http_handler/user_submission_ban_query_handler.hpp"

#include <array>

namespace{
    using endpoint_descriptor = user_route_groups::endpoint_descriptor;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array public_user_list_pattern{
        path_segment_matcher::literal("list")
    };
    inline constexpr std::array me_pattern{
        path_segment_matcher::literal("me")
    };
    inline constexpr std::array me_statistics_pattern{
        path_segment_matcher::literal("me"),
        path_segment_matcher::literal("statistics")
    };
    inline constexpr std::array me_submission_ban_pattern{
        path_segment_matcher::literal("me"),
        path_segment_matcher::literal("submission-ban")
    };
    inline constexpr std::array me_solved_problems_pattern{
        path_segment_matcher::literal("me"),
        path_segment_matcher::literal("solved-problems")
    };
    inline constexpr std::array me_wrong_problems_pattern{
        path_segment_matcher::literal("me"),
        path_segment_matcher::literal("wrong-problems")
    };
    inline constexpr std::array user_summary_by_login_id_pattern{
        path_segment_matcher::literal("id"),
        path_segment_matcher::percent_decoded_string("user_login_id")
    };
    inline constexpr std::array user_id_pattern{
        path_segment_matcher::positive_int64("user_id")
    };
    inline constexpr std::array user_statistics_pattern{
        path_segment_matcher::positive_int64("user_id"),
        path_segment_matcher::literal("statistics")
    };
    inline constexpr std::array user_solved_problems_pattern{
        path_segment_matcher::positive_int64("user_id"),
        path_segment_matcher::literal("solved-problems")
    };
    inline constexpr std::array user_wrong_problems_pattern{
        path_segment_matcher::positive_int64("user_id"),
        path_segment_matcher::literal("wrong-problems")
    };
    inline constexpr std::array user_submission_ban_pattern{
        path_segment_matcher::positive_int64("user_id"),
        path_segment_matcher::literal("submission-ban")
    };

    const auto& route_table(){
        static const std::array<endpoint_descriptor, 13> route_table_value{{
            endpoint_descriptor{
                .name = "get_user_list",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = http_route::empty_path_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match&) -> user_router::response_type {
                    return user_query_handler::get_user_list(context_value);
                }
            },
            endpoint_descriptor{
                .name = "get_public_user_list",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = public_user_list_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match&) -> user_router::response_type {
                    return user_query_handler::get_public_user_list(context_value);
                }
            },
            endpoint_descriptor{
                .name = "get_me",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = me_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match&) -> user_router::response_type {
                    return user_query_handler::get_me(context_value);
                }
            },
            endpoint_descriptor{
                .name = "get_me_submission_statistics",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = me_statistics_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match&) -> user_router::response_type {
                    return user_query_handler::get_me_submission_statistics(
                        context_value
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_me_submission_ban",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = me_submission_ban_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match&) -> user_router::response_type {
                    return user_query_handler::get_me_submission_ban(context_value);
                }
            },
            endpoint_descriptor{
                .name = "get_me_solved_problems",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = me_solved_problems_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match&) -> user_router::response_type {
                    return user_query_handler::get_me_solved_problems(context_value);
                }
            },
            endpoint_descriptor{
                .name = "get_me_wrong_problems",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = me_wrong_problems_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match&) -> user_router::response_type {
                    return user_query_handler::get_me_wrong_problems(context_value);
                }
            },
            endpoint_descriptor{
                .name = "get_user_summary_by_login_id",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = user_summary_by_login_id_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_query_handler::get_user_summary_by_login_id(
                        context_value,
                        route_match_value.string_param("user_login_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_user_submission_statistics",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = user_statistics_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_query_handler::get_user_submission_statistics(
                        context_value,
                        route_match_value.int64_param("user_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_user_solved_problems",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = user_solved_problems_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_query_handler::get_user_solved_problems(
                        context_value,
                        route_match_value.int64_param("user_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_user_wrong_problems",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = user_wrong_problems_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_query_handler::get_user_wrong_problems(
                        context_value,
                        route_match_value.int64_param("user_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_user_summary",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = user_id_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_query_handler::get_user_summary(
                        context_value,
                        route_match_value.int64_param("user_id")
                    );
                }
            },
            endpoint_descriptor{
                .name = "get_user_submission_ban",
                .method = http_verb::get,
                .kind = http_route::operation_kind::query,
                .pattern = user_submission_ban_pattern,
                .invoke = [](user_router::context_type& context_value,
                    const http_route::route_match& route_match_value)
                    -> user_router::response_type {
                    return user_query_handler::get_user_submission_ban(
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
user_route_groups::query_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
