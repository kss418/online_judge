#pragma once
#include "common/error_code.hpp"
#include "dto/auth_dto.hpp"

#include <pqxx/pqxx>

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>

namespace auth_util{
    std::expected<void, error_code> insert_token(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        const auth_dto::hashed_token& hashed_token_value,
        std::chrono::seconds token_ttl
    );
    
    std::expected<bool, error_code> revoke_token(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_token& hashed_token_value
    );

    std::expected<std::optional<auth_dto::identity>, error_code> get_token_identity(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_token& hashed_token_value
    );

    std::expected<void, error_code> update_last_used_at(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_token& hashed_token_value
    );

    std::expected<bool, error_code> update_permission_level(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int32_t permission_level
    );

    std::expected<auth_dto::user_summary_list, error_code> get_user_list(
        pqxx::transaction_base& transaction
    );

    std::expected<bool, error_code> update_expires_at(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_token& hashed_token_value,
        std::chrono::seconds token_ttl
    );
}
