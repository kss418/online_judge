#include "dto/auth_dto.hpp"

#include "common/password_util.hpp"
#include "http_server/http_util.hpp"

std::optional<auth_dto::credentials> auth_dto::make_credentials_from_json(
    const boost::json::object& json
){
    const auto user_login_id_opt = http_util::get_non_empty_string_field(
        json,
        "user_login_id"
    );
    const auto raw_password_opt = http_util::get_non_empty_string_field(
        json,
        "raw_password"
    );
    if(!user_login_id_opt || !raw_password_opt){
        return std::nullopt;
    }

    credentials credentials_value;
    credentials_value.user_login_id = std::string{*user_login_id_opt};
    credentials_value.raw_password = std::string{*raw_password_opt};
    return credentials_value;
}

std::expected<auth_dto::hashed_credentials, error_code> auth_dto::hash_credentials(
    const credentials& credentials_value
){
    if(
        credentials_value.user_login_id.empty() ||
        credentials_value.raw_password.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
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
