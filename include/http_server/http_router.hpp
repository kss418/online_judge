#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "http_server/http_handler.hpp"

#include <array>
#include <boost/beast/http/verb.hpp>

#include <expected>
#include <optional>
#include <string_view>

class http_router{
public:
    using request_type = http_handler::request_type;
    using response_type = http_handler::response_type;

    http_router(const http_router&) = delete;
    http_router& operator=(const http_router&) = delete;
    http_router(http_router&&) noexcept = default;
    http_router& operator=(http_router&&) noexcept = default;

    static std::expected<http_router, error_code> create(db_connection db_connection);
    response_type handle(const request_type& request);
private:
    using route_handler = response_type (http_handler::*)(const request_type&);

    struct route_definition{
        boost::beast::http::verb method;
        std::string_view path;
        route_handler handler;
    };

    explicit http_router(http_handler&& http_handler);

    static std::optional<route_handler> find_route_handler(
        boost::beast::http::verb method,
        std::string_view path
    );
    static bool has_route_path(std::string_view path);

    static constexpr std::string_view health_path_ = "/api/health";
    static constexpr std::string_view sign_up_path_ = "/api/sign-up";
    static constexpr std::string_view login_path_ = "/api/login";
    static constexpr std::string_view token_renew_path_ = "/api/token/renew";
    static constexpr std::string_view logout_path_ = "/api/logout";
    static constexpr std::array<route_definition, 5> routes_{{
        route_definition{
            boost::beast::http::verb::get,
            health_path_,
            &http_handler::handle_health_get
        },
        route_definition{
            boost::beast::http::verb::post,
            sign_up_path_,
            &http_handler::handle_sign_up_post
        },
        route_definition{
            boost::beast::http::verb::post,
            login_path_,
            &http_handler::handle_login_post
        },
        route_definition{
            boost::beast::http::verb::post,
            token_renew_path_,
            &http_handler::handle_token_renew_post
        },
        route_definition{
            boost::beast::http::verb::post,
            logout_path_,
            &http_handler::handle_logout_post
        }
    }};

    http_handler http_handler_;
};
