#pragma once

#include "http_router/auth_router.hpp"
#include "http_router/problem_router.hpp"
#include "http_router/submission_router.hpp"
#include "http_router/system_router.hpp"
#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <expected>
#include <optional>
#include <string_view>

class http_dispatcher{
public:
    using request_type = auth_router::request_type;
    using response_type = auth_router::response_type;

    http_dispatcher(const http_dispatcher&) = delete;
    http_dispatcher& operator=(const http_dispatcher&) = delete;
    http_dispatcher(http_dispatcher&& other) noexcept;
    http_dispatcher& operator=(http_dispatcher&&) noexcept = delete;

    static std::expected<http_dispatcher, error_code> create(db_connection db_connection);
    response_type handle(const request_type& request);
private:
    explicit http_dispatcher(db_connection db_connection);

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
    auth_router auth_router_;
    problem_router problem_router_;
    submission_router submission_router_;
    system_router system_router_;
};
