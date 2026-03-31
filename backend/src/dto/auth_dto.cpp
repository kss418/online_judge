#include "dto/auth_dto.hpp"

#include "common/password_util.hpp"
#include "common/json_field_util.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace{
    constexpr std::size_t credential_min_length = 4;
    constexpr std::size_t credential_max_length = 15;

    std::optional<dto_validation_error> validate_credential_length(
        std::string_view field_name,
        std::string_view value
    ){
        if(
            value.size() < credential_min_length ||
            value.size() > credential_max_length
        ){
            return dto_validation_error{
                .code = "invalid_length",
                .message = std::string{field_name} + " must be between 4 and 15 characters",
                .field_opt = std::string{field_name}
            };
        }

        return std::nullopt;
    }
}

std::expected<auth_dto::sign_up_request, dto_validation_error>
auth_dto::make_sign_up_request_from_json(const boost::json::object& json){
    const auto credentials_exp = make_credentials_from_json(json);
    if(!credentials_exp){
        return std::unexpected(credentials_exp.error());
    }

    sign_up_request sign_up_request_value;
    sign_up_request_value.user_login_id = credentials_exp->user_login_id;
    sign_up_request_value.raw_password = credentials_exp->raw_password;
    return sign_up_request_value;
}

std::expected<auth_dto::credentials, dto_validation_error> auth_dto::make_credentials_from_json(
    const boost::json::object& json
){
    const auto user_login_id_opt = json_field_util::get_non_empty_string_field(
        json,
        "user_login_id"
    );
    if(!user_login_id_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: user_login_id",
            .field_opt = "user_login_id"
        });
    }

    const auto raw_password_opt = json_field_util::get_non_empty_string_field(
        json,
        "raw_password"
    );
    if(!raw_password_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: raw_password",
            .field_opt = "raw_password"
        });
    }

    const auto user_login_id_length_error_opt = validate_credential_length(
        "user_login_id",
        *user_login_id_opt
    );
    if(user_login_id_length_error_opt){
        return std::unexpected(*user_login_id_length_error_opt);
    }

    const auto raw_password_length_error_opt = validate_credential_length(
        "raw_password",
        *raw_password_opt
    );
    if(raw_password_length_error_opt){
        return std::unexpected(*raw_password_length_error_opt);
    }

    credentials credentials_value;
    credentials_value.user_login_id = std::string{*user_login_id_opt};
    credentials_value.raw_password = std::string{*raw_password_opt};
    return credentials_value;
}

std::expected<auth_dto::hashed_sign_up_request, error_code> auth_dto::hash_sign_up_request(
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
