#pragma once
#include "common/repository_error.hpp"
#include "dto/auth_dto.hpp"

#include <pqxx/pqxx>

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>

namespace auth_repository{
    std::expected<void, repository_error> insert_token(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        const auth_dto::hashed_token& hashed_token_value,
        std::chrono::seconds token_ttl
    );
    
    std::expected<bool, repository_error> revoke_token(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_token& hashed_token_value
    );

    std::expected<std::optional<auth_dto::identity>, repository_error> get_token_identity(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_token& hashed_token_value
    );

    std::expected<bool, repository_error> update_permission_level(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int32_t permission_level
    );

    std::expected<auth_dto::user_summary_list, repository_error> get_user_list(
        pqxx::transaction_base& transaction
    );

    std::expected<bool, repository_error> update_expires_at(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_token& hashed_token_value,
        std::chrono::seconds token_ttl
    );
}
