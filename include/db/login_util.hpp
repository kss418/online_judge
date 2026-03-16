#pragma once
#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <string_view>

namespace pqxx{
class transaction_base;
}

namespace login_util{
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
}
