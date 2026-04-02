#include "dto/auth_dto.hpp"

#include "common/password_util.hpp"

#include <string_view>

namespace{
    constexpr std::size_t credential_min_length = 4;
    constexpr std::size_t credential_max_length = 15;

    bool is_valid_credential_value(std::string_view value){
        return
            value.size() >= credential_min_length &&
            value.size() <= credential_max_length;
    }

}

bool auth_dto::is_valid(const token& token_value){
    return !token_value.value.empty();
}

bool auth_dto::is_valid(const hashed_token& hashed_token_value){
    return !hashed_token_value.token_hash.empty();
}

bool auth_dto::is_valid(const sign_up_request& sign_up_request_value){
    return
        is_valid_credential_value(sign_up_request_value.user_login_id) &&
        is_valid_credential_value(sign_up_request_value.raw_password);
}

bool auth_dto::is_valid(const credentials& credentials_value){
    return
        is_valid_credential_value(credentials_value.user_login_id) &&
        is_valid_credential_value(credentials_value.raw_password);
}

bool auth_dto::is_valid(const hashed_sign_up_request& hashed_sign_up_request_value){
    return
        !hashed_sign_up_request_value.user_login_id.empty() &&
        !hashed_sign_up_request_value.password_hash.empty();
}

bool auth_dto::is_valid(const hashed_credentials& hashed_credentials_value){
    return
        !hashed_credentials_value.user_login_id.empty() &&
        !hashed_credentials_value.password_hash.empty();
}

std::expected<auth_dto::hashed_sign_up_request, infra_error> auth_dto::hash_sign_up_request(
    const sign_up_request& sign_up_request_value
){
    auth_dto::credentials credentials_value;
    credentials_value.user_login_id = sign_up_request_value.user_login_id;
    credentials_value.raw_password = sign_up_request_value.raw_password;
    const auto hashed_credentials_exp = hash_credentials(credentials_value);
    if(!hashed_credentials_exp){
        return std::unexpected(hashed_credentials_exp.error());
    }

    hashed_sign_up_request hashed_sign_up_request_value;
    hashed_sign_up_request_value.user_login_id = hashed_credentials_exp->user_login_id;
    hashed_sign_up_request_value.password_hash = hashed_credentials_exp->password_hash;
    return hashed_sign_up_request_value;
}

std::expected<auth_dto::hashed_credentials, infra_error> auth_dto::hash_credentials(
    const credentials& credentials_value
){
    if(!is_valid(credentials_value)){
        return std::unexpected(infra_error::invalid_argument);
    }

    const auto password_hash_exp = password_util::get_password_hash(
        credentials_value.raw_password
    );
    if(!password_hash_exp){
        return std::unexpected(password_hash_exp.error());
    }

    hashed_credentials hashed_credentials_value;
    hashed_credentials_value.user_login_id = credentials_value.user_login_id;
    hashed_credentials_value.password_hash = *password_hash_exp;
    return hashed_credentials_value;
}
