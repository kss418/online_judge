#include "http_router/auth_router.hpp"

#include "http_router/route_table.hpp"

#include <array>

namespace{
    using endpoint_descriptor = http_route::endpoint_descriptor<
        auth_router::context_type,
        auth_router::response_type
    >;
    using path_segment_matcher = http_route::path_segment_matcher;
    using http_verb = boost::beast::http::verb;

    inline constexpr std::array sign_up_pattern{
        path_segment_matcher::literal("sign-up")
    };
    inline constexpr std::array login_pattern{
        path_segment_matcher::literal("login")
    };
    inline constexpr std::array token_renew_pattern{
        path_segment_matcher::literal("token"),
        path_segment_matcher::literal("renew")
    };
    inline constexpr std::array logout_pattern{
        path_segment_matcher::literal("logout")
    };
}

auth_router::response_type auth_router::route(
    context_type& context,
    std::string_view path
){
    static const std::array<endpoint_descriptor, 4> auth_route_table{{
        endpoint_descriptor{
            .name = "post_sign_up",
            .method = http_verb::post,
            .kind = http_route::operation_kind::command,
            .pattern = sign_up_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return auth_command_handler::post_sign_up(context_value);
            }
        },
        endpoint_descriptor{
            .name = "post_login",
            .method = http_verb::post,
            .kind = http_route::operation_kind::command,
            .pattern = login_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return auth_command_handler::post_login(context_value);
            }
        },
        endpoint_descriptor{
            .name = "post_token_renew",
            .method = http_verb::post,
            .kind = http_route::operation_kind::command,
            .pattern = token_renew_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return auth_command_handler::post_token_renew(context_value);
            }
        },
        endpoint_descriptor{
            .name = "post_logout",
            .method = http_verb::post,
            .kind = http_route::operation_kind::command,
            .pattern = logout_pattern,
            .invoke = [](context_type& context_value,
                const http_route::route_match&) -> response_type {
                return auth_command_handler::post_logout(context_value);
            }
        }
    }};

    return http_route::dispatch_route_table(
        context,
        path,
        auth_route_table
    );
}
