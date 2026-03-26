#include "dto/auth_dto.hpp"

#include "common/permission_util.hpp"
#include "common/password_util.hpp"
#include "http_server/http_util.hpp"

std::expected<auth_dto::sign_up_request, dto_validation_error>
auth_dto::make_sign_up_request_from_json(const boost::json::object& json){
    const auto user_name_opt = http_util::get_non_empty_string_field(
        json,
        "user_name"
    );
    if(!user_name_opt){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: user_name",
            .field_opt = "user_name"
        });
    }

    const auto credentials_exp = make_credentials_from_json(json);
    if(!credentials_exp){
        return std::unexpected(credentials_exp.error());
    }

    sign_up_request sign_up_request_value;
    sign_up_request_value.user_name = std::string{*user_name_opt};
    sign_up_request_value.user_login_id = credentials_exp->user_login_id;
    sign_up_request_value.raw_password = credentials_exp->raw_password;
    return sign_up_request_value;
}

std::expected<auth_dto::credentials, dto_validation_error> auth_dto::make_credentials_from_json(
    const boost::json::object& json
){
    const auto user_login_id_opt = http_util::get_non_empty_string_field(
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

    const auto raw_password_opt = http_util::get_non_empty_string_field(
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

    credentials credentials_value;
    credentials_value.user_login_id = std::string{*user_login_id_opt};
    credentials_value.raw_password = std::string{*raw_password_opt};
    return credentials_value;
}

std::expected<auth_dto::permission_update_request, dto_validation_error>
auth_dto::make_permission_update_request_from_json(const boost::json::object& json){
    if(json.if_contains("permission_level") == nullptr){
        return std::unexpected(dto_validation_error{
            .code = "missing_field",
            .message = "required field: permission_level",
            .field_opt = "permission_level"
        });
    }

    const auto permission_level_opt = http_util::get_non_negative_int32_field(
        json,
        "permission_level"
    );
    if(!permission_level_opt){
        return std::unexpected(dto_validation_error{
            .code = "invalid_field",
            .message = "permission_level must be one of 0, 1, 2",
            .field_opt = "permission_level"
        });
    }

    if(!permission_util::is_valid_permission_level(*permission_level_opt)){
        return std::unexpected(dto_validation_error{
            .code = "invalid_field",
            .message = "permission_level must be one of 0, 1, 2",
            .field_opt = "permission_level"
        });
    }

    permission_update_request permission_update_request_value;
    permission_update_request_value.permission_level = *permission_level_opt;
    return permission_update_request_value;
}

std::expected<auth_dto::hashed_sign_up_request, error_code> auth_dto::hash_sign_up_request(
    const sign_up_request& sign_up_request_value
){
    if(sign_up_request_value.user_name.empty()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    auth_dto::credentials credentials_value;
    credentials_value.user_login_id = sign_up_request_value.user_login_id;
    credentials_value.raw_password = sign_up_request_value.raw_password;
    const auto hashed_credentials_exp = hash_credentials(credentials_value);
    if(!hashed_credentials_exp){
        return std::unexpected(hashed_credentials_exp.error());
    }

    hashed_sign_up_request hashed_sign_up_request_value;
    hashed_sign_up_request_value.user_name = sign_up_request_value.user_name;
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
