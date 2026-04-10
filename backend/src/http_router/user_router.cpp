#include "http_router/user_router.hpp"

#include "http_router/route_table.hpp"

#include <array>

namespace{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        user_router,
        user_router::context_type,
        user_router::response_type
    >;
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
}

user_router::response_type user_router::route(
    context_type& context,
    std::string_view path
){
    static const std::array<endpoint_descriptor, 17> user_route_table{{
        endpoint_descriptor{
            .name = "get_user_list",
            .method = http_verb::get,
            .pattern = http_route::empty_path_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match&) -> response_type {
                return user_handler::get_user_list(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_public_user_list",
            .method = http_verb::get,
            .pattern = public_user_list_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match&) -> response_type {
                return user_handler::get_public_user_list(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_me",
            .method = http_verb::get,
            .pattern = me_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match&) -> response_type {
                return user_handler::get_me(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_me_submission_statistics",
            .method = http_verb::get,
            .pattern = me_statistics_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match&) -> response_type {
                return user_handler::get_me_submission_statistics(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_me_submission_ban",
            .method = http_verb::get,
            .pattern = me_submission_ban_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match&) -> response_type {
                return user_handler::get_me_submission_ban(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_me_solved_problems",
            .method = http_verb::get,
            .pattern = me_solved_problems_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match&) -> response_type {
                return user_handler::get_me_solved_problems(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_me_wrong_problems",
            .method = http_verb::get,
            .pattern = me_wrong_problems_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match&) -> response_type {
                return user_handler::get_me_wrong_problems(context_value);
            }
        },
        endpoint_descriptor{
            .name = "get_user_summary_by_login_id",
            .method = http_verb::get,
            .pattern = user_summary_by_login_id_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::get_user_summary_by_login_id(
                    context_value,
                    route_match_value.string_param("user_login_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_user_submission_statistics",
            .method = http_verb::get,
            .pattern = user_statistics_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::get_user_submission_statistics(
                    context_value,
                    route_match_value.int64_param("user_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_user_solved_problems",
            .method = http_verb::get,
            .pattern = user_solved_problems_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::get_user_solved_problems(
                    context_value,
                    route_match_value.int64_param("user_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_user_wrong_problems",
            .method = http_verb::get,
            .pattern = user_wrong_problems_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::get_user_wrong_problems(
                    context_value,
                    route_match_value.int64_param("user_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_user_summary",
            .method = http_verb::get,
            .pattern = user_id_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::get_user_summary(
                    context_value,
                    route_match_value.int64_param("user_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "put_user_admin",
            .method = http_verb::put,
            .pattern = user_admin_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::put_user_admin(
                    context_value,
                    route_match_value.int64_param("user_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "put_user_regular",
            .method = http_verb::put,
            .pattern = user_regular_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::put_user_regular(
                    context_value,
                    route_match_value.int64_param("user_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "get_user_submission_ban",
            .method = http_verb::get,
            .pattern = user_submission_ban_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::get_user_submission_ban(
                    context_value,
                    route_match_value.int64_param("user_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "post_user_submission_ban",
            .method = http_verb::post,
            .pattern = user_submission_ban_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::post_user_submission_ban(
                    context_value,
                    route_match_value.int64_param("user_id")
                );
            }
        },
        endpoint_descriptor{
            .name = "delete_user_submission_ban",
            .method = http_verb::delete_,
            .pattern = user_submission_ban_pattern,
            .invoke = [](user_router&,
                context_type& context_value,
                const http_route::route_match& route_match_value) -> response_type {
                return user_handler::delete_user_submission_ban(
                    context_value,
                    route_match_value.int64_param("user_id")
                );
            }
        }
    }};

    return http_route::dispatch_route_table(
        *this,
        context,
        path,
        user_route_table
    );
}
