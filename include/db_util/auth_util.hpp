#pragma once
#include "common/error_code.hpp"

#include <pqxx/pqxx>

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace auth_util{
    struct token_identity{
        std::int64_t user_id = 0;
        bool is_admin = false;
    };

    std::expected<void, error_code> insert_token(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::string_view token_hash,
        std::chrono::seconds token_ttl
    );
    
    std::expected<bool, error_code> revoke_token(
        pqxx::transaction_base& transaction,
        std::string_view token_hash
    );

    std::expected<std::optional<token_identity>, error_code> get_token_identity(
        pqxx::transaction_base& transaction,
        std::string_view token_hash
    );

    std::expected<void, error_code> update_last_used_at(
        pqxx::transaction_base& transaction,
        std::string_view token_hash
    );

    std::expected<bool, error_code> update_admin_status(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        bool is_admin
    );

    std::expected<bool, error_code> update_expires_at(
        pqxx::transaction_base& transaction,
        std::string_view token_hash,
        std::chrono::seconds token_ttl
    );
}
