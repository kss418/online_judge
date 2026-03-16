#pragma once
#include "common/error_code.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace pqxx{
class transaction_base;
}

namespace login_util{
    struct login_user{
        std::int64_t user_id = 0;
        std::string user_login_id;
        std::string user_password_hash;
        bool is_admin = false;
    };

    std::expected<std::int64_t, error_code> create_user(
        pqxx::transaction_base& transaction,
        std::string_view user_login_id,
        std::string_view user_password_hash
    );

    std::expected<std::optional<login_user>, error_code> find_user_by_login_id(
        pqxx::transaction_base& transaction,
        std::string_view user_login_id
    );
}
