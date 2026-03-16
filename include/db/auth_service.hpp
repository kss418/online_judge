#pragma once
#include "common/error_code.hpp"
#include "db/db_service_base.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

struct auth_identity{
    std::int64_t user_id = 0;
    bool is_admin = false;
};

class auth_service : public db_service_base<auth_service>{
public:
    std::expected<std::string, error_code> issue_token(
        std::int64_t user_id,
        bool is_admin
    );
    std::expected<auth_identity, error_code> authenticate_token(std::string_view token);
    std::expected<void, error_code> revoke_token(std::string_view token);

private:
    friend class db_service_base<auth_service>;

    explicit auth_service(db_connection connection);
};
