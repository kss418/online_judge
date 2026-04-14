#include "http_router/auth_route_groups.hpp"

#include "http_handler/auth_command_handler.hpp"

#include <array>

namespace{
    using endpoint_descriptor = auth_route_groups::endpoint_descriptor;
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

    const auto& route_table(){
        static const std::array<endpoint_descriptor, 4> route_table_value{{
            endpoint_descriptor{
                .name = "post_sign_up",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = sign_up_pattern,
                .invoke = [](auth_router::context_type& context_value,
                    const http_route::route_match&)
                    -> auth_router::response_type {
                    return auth_command_handler::post_sign_up(context_value);
                }
            },
            endpoint_descriptor{
                .name = "post_login",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = login_pattern,
                .invoke = [](auth_router::context_type& context_value,
                    const http_route::route_match&)
                    -> auth_router::response_type {
                    return auth_command_handler::post_login(context_value);
                }
            },
            endpoint_descriptor{
                .name = "post_token_renew",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = token_renew_pattern,
                .invoke = [](auth_router::context_type& context_value,
                    const http_route::route_match&)
                    -> auth_router::response_type {
                    return auth_command_handler::post_token_renew(context_value);
                }
            },
            endpoint_descriptor{
                .name = "post_logout",
                .method = http_verb::post,
                .kind = http_route::operation_kind::command,
                .pattern = logout_pattern,
                .invoke = [](auth_router::context_type& context_value,
                    const http_route::route_match&)
                    -> auth_router::response_type {
                    return auth_command_handler::post_logout(context_value);
                }
            }
        }};

        return route_table_value;
    }
}

std::span<const auth_route_groups::endpoint_descriptor>
auth_route_groups::command_routes(){
    return std::span<const endpoint_descriptor>{route_table()};
}
