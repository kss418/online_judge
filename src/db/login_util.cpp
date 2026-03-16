#include "db/login_util.hpp"

std::expected<std::int64_t, error_code> login_util::create_user(
    pqxx::transaction_base& transaction,
    std::string_view user_login_id,
    std::string_view user_password_hash
){
    static_cast<void>(transaction);

    if(user_login_id.empty() || user_password_hash.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return std::unexpected(error_code::create(errno_error::operation_not_supported));
}

std::expected<std::optional<login_util::login_user>, error_code> login_util::find_user_by_login_id(
    pqxx::transaction_base& transaction,
    std::string_view user_login_id
){
    static_cast<void>(transaction);

    if(user_login_id.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return std::unexpected(error_code::create(errno_error::operation_not_supported));
}
