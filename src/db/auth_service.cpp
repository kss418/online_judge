#include "db/auth_service.hpp"

#include <utility>

auth_service::auth_service(db_connection connection) :
    db_service_base<auth_service>(std::move(connection)){}

std::expected<std::string, error_code> auth_service::issue_token(
    std::int64_t user_id,
    bool is_admin
){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(user_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    static_cast<void>(is_admin);
    return std::unexpected(error_code::create(errno_error::operation_not_supported));
}

std::expected<auth_identity, error_code> auth_service::authenticate_token(std::string_view token){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(token.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return std::unexpected(error_code::create(errno_error::operation_not_supported));
}

std::expected<void, error_code> auth_service::revoke_token(std::string_view token){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(token.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return std::unexpected(error_code::create(errno_error::operation_not_supported));
}
