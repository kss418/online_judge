#pragma once
#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace pqxx{
    class transaction_base;
}

namespace login_util{
    struct login_identity{
        std::int64_t user_id = 0;
        bool is_admin = false;
    };

    std::expected<std::int64_t, error_code> create_user(
        pqxx::transaction_base& transaction,
        std::string_view user_login_id,
        std::string_view user_password_hash
    );

    std::expected<bool, error_code> verify_user(
        pqxx::transaction_base& transaction,
        std::string_view user_login_id,
        std::string_view user_password_hash
    );
    std::expected<std::optional<login_identity>, error_code> get_login_identity(
        pqxx::transaction_base& transaction,
        std::string_view user_login_id,
        std::string_view user_password_hash
    );
}
