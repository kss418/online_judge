#pragma once
#include "error/repository_error.hpp"
#include "dto/auth_dto.hpp"

#include <expected>
#include <optional>

namespace pqxx{
    class transaction_base;
}

namespace login_repository{
    std::expected<std::int64_t, repository_error> create_user(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_sign_up_request& sign_up_request_value
    );

    std::expected<bool, repository_error> verify_user(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_credentials& credentials_value
    );
    std::expected<std::optional<auth_dto::identity>, repository_error> get_login_identity(
        pqxx::transaction_base& transaction,
        const auth_dto::hashed_credentials& credentials_value
    );
}
