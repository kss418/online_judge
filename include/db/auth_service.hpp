#pragma once
#include "common/error_code.hpp"
#include "db/db_service_base.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

struct auth_identity{
    std::int64_t user_id = 0;
    bool is_admin = false;
};

class auth_service : public db_service_base<auth_service>{
public:
    std::expected<std::string, error_code> issue_token(std::int64_t user_id);
    std::expected<std::optional<auth_identity>, error_code> auth_token(std::string_view token);
    std::expected<bool, error_code> renew_token(std::string_view token);
    std::expected<bool, error_code> revoke_token(std::string_view token);

private:
    friend class db_service_base<auth_service>;

    static constexpr std::chrono::seconds TOKEN_TTL{std::chrono::hours{24 * 30}};
    static constexpr int ISSUE_MAX_ATTEMPTS = 3;

    explicit auth_service(db_connection connection);
};
