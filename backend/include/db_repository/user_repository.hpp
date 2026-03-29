#pragma once

#include "common/error_code.hpp"
#include "dto/user_dto.hpp"

#include <pqxx/pqxx>

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace user_repository{
    std::expected<user_dto::list, error_code> get_public_list(
        pqxx::transaction_base& transaction,
        const user_dto::list_filter& filter_value
    );
    std::expected<std::optional<user_dto::summary>, error_code> get_summary(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );
    std::expected<std::optional<user_dto::summary>, error_code> get_summary_by_login_id(
        pqxx::transaction_base& transaction,
        std::string_view user_login_id
    );
    std::expected<std::optional<std::string>, error_code> create_submission_ban(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::int32_t duration_minutes
    );
    std::expected<bool, error_code> update_submission_banned_until(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::string_view submission_banned_until
    );
    std::expected<bool, error_code> clear_submission_banned_until(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );
}
