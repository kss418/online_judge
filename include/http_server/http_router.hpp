#pragma once

#include "http_handler/auth_handler.hpp"
#include "http_handler/problem_handler.hpp"
#include "http_handler/submission_handler.hpp"
#include "http_handler/system_handler.hpp"
#include "common/error_code.hpp"
#include "db/db_connection.hpp"

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
    explicit http_router(db_connection db_connection);

    static std::optional<std::string_view> strip_path_prefix(
        std::string_view prefix_path,
        std::string_view path
    );
    std::optional<response_type> try_handle_route(const request_type& request);

    static constexpr std::string_view system_path_prefix_ = "/api/system";
    static constexpr std::string_view auth_path_prefix_ = "/api/auth";
    static constexpr std::string_view submission_path_prefix_ = "/api/submission";
    static constexpr std::string_view problem_path_prefix_ = "/api/problem";

    db_connection db_connection_;
    system_handler system_handler_;
    auth_handler auth_handler_;
    submission_handler submission_handler_;
    problem_handler problem_handler_;
};
