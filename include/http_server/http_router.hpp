#pragma once

#include "http_handler/auth_handler.hpp"
#include "http_handler/submission_handler.hpp"
#include "http_handler/system_handler.hpp"
#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <boost/beast/http/verb.hpp>

#include <expected>
#include <optional>
#include <string_view>

class http_router{
public:
    using request_type = auth_handler::request_type;
    using response_type = auth_handler::response_type;

    http_router(const http_router&) = delete;
    http_router& operator=(const http_router&) = delete;
    http_router(http_router&& other) noexcept;
    http_router& operator=(http_router&&) noexcept = delete;

    static std::expected<http_router, error_code> create(db_connection db_connection);
    response_type handle(const request_type& request);
private:
    template <typename handler_type>
    using route_handler = response_type (handler_type::*)(const request_type&);

    template <typename handler_type>
    struct route_definition{
        boost::beast::http::verb method;
        std::string_view path;
        route_handler<handler_type> handler;
    };

    explicit http_router(db_connection db_connection);

    static std::optional<route_handler<system_handler>> find_system_route_handler(
        boost::beast::http::verb method,
        std::string_view path
    );
    static std::optional<route_handler<auth_handler>> find_auth_route_handler(
        boost::beast::http::verb method,
        std::string_view path
    );
    static std::optional<route_handler<submission_handler>> find_submission_route_handler(
        boost::beast::http::verb method,
        std::string_view path
    );
    std::optional<response_type> try_handle_route(const request_type& request);
    static bool has_route_path(std::string_view path);

    static constexpr std::string_view health_path_ = "/api/system/health";
    static constexpr std::string_view sign_up_path_ = "/api/auth/sign-up";
    static constexpr std::string_view login_path_ = "/api/auth/login";
    static constexpr std::string_view token_renew_path_ = "/api/auth/token/renew";
    static constexpr std::string_view logout_path_ = "/api/auth/logout";
    static constexpr std::string_view create_submission_path_ = "/api/submission";

    static constexpr std::array<route_definition<system_handler>, 1> system_routes_{{
        route_definition<system_handler>{
            boost::beast::http::verb::get,
            health_path_,
            &system_handler::handle_health_get
        }
    }};

    static constexpr std::array<route_definition<auth_handler>, 4> auth_routes_{{
        route_definition<auth_handler>{
            boost::beast::http::verb::post,
            sign_up_path_,
            &auth_handler::handle_sign_up_post
        },
        route_definition<auth_handler>{
            boost::beast::http::verb::post,
            login_path_,
            &auth_handler::handle_login_post
        },
        route_definition<auth_handler>{
            boost::beast::http::verb::post,
            token_renew_path_,
            &auth_handler::handle_token_renew_post
        },
        route_definition<auth_handler>{
            boost::beast::http::verb::post,
            logout_path_,
            &auth_handler::handle_logout_post
        }
    }};

    static constexpr std::array<route_definition<submission_handler>, 1> submission_routes_{{
        route_definition<submission_handler>{
            boost::beast::http::verb::post,
            create_submission_path_,
            &submission_handler::handle_create_submission_post
        }
    }};

    db_connection db_connection_;
    system_handler system_handler_;
    auth_handler auth_handler_;
    submission_handler submission_handler_;
};
