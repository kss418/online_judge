#pragma once

#include "common/error_code.hpp"
#include "dto/user_dto.hpp"

#include <pqxx/pqxx>

#include <cstdint>
#include <expected>
#include <optional>

namespace user_repository{
    std::expected<std::optional<user_dto::summary>, error_code> get_summary(
        pqxx::transaction_base& transaction,
        std::int64_t user_id
    );
    std::expected<std::optional<user_dto::summary>, error_code> get_summary_by_login_id(
        pqxx::transaction_base& transaction,
        std::string_view user_login_id
    );
}
