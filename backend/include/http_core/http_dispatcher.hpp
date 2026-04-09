#pragma once

#include "common/db_connection_pool.hpp"
#include "http_core/request_context.hpp"
#include "http_core/request_observer.hpp"
#include "http_router/auth_router.hpp"
#include "http_router/problem_router.hpp"
#include "http_router/submission_router.hpp"
#include "http_router/system_router.hpp"
#include "http_router/user_router.hpp"

#include <expected>
#include <optional>
#include <string_view>

class http_dispatcher{
public:
    using request_type = auth_router::request_type;
    using response_type = auth_router::response_type;

    http_dispatcher(const http_dispatcher&) = delete;
    http_dispatcher& operator=(const http_dispatcher&) = delete;
    http_dispatcher(http_dispatcher&&) noexcept = delete;
    http_dispatcher& operator=(http_dispatcher&&) noexcept = delete;

    explicit http_dispatcher(
        db_connection_pool& db_connection_pool,
        request_observer* request_observer = nullptr
    );
    response_type handle(const request_type& request);
private:
    static std::optional<std::string_view> strip_path_prefix(
        std::string_view prefix_path,
        std::string_view path
    );
    static bool has_db_route_prefix(std::string_view path);
    std::optional<response_type> try_handle_system_route(
        request_context& context,
        std::string_view path
    );
    std::optional<response_type> try_handle_route(
        request_context& context,
        std::string_view path
    );

    static constexpr std::string_view system_path_prefix_ = "/api/system";
    static constexpr std::string_view auth_path_prefix_ = "/api/auth";
    static constexpr std::string_view submission_path_prefix_ = "/api/submission";
    static constexpr std::string_view problem_path_prefix_ = "/api/problem";
    static constexpr std::string_view user_path_prefix_ = "/api/user";

    db_connection_pool& db_connection_pool_;
    request_observer* request_observer_ = nullptr;
    system_router system_router_;
};
