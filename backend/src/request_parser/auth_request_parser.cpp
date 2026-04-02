#include "request_parser/auth_request_parser.hpp"

#include "common/json_field_util.hpp"
#include "error/request_error.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace{
    constexpr std::size_t credential_min_length = 4;
    constexpr std::size_t credential_max_length = 15;

    std::optional<http_error> validate_credential_length(
        std::string_view field_name,
        std::string_view value
    ){
        if(
            value.size() < credential_min_length ||
            value.size() > credential_max_length
        ){
            return request_error::make_invalid_length_error(
                field_name,
                std::string{field_name} + " must be between 4 and 15 characters"
            );
        }

        return std::nullopt;
    }
}

std::expected<auth_dto::sign_up_request, http_error>
auth_request_parser::parse_sign_up_request(const boost::json::object& json){
    const auto credentials_exp = parse_credentials(json);
    if(!credentials_exp){
        return std::unexpected(credentials_exp.error());
    }

    auth_dto::sign_up_request sign_up_request_value;
    sign_up_request_value.user_login_id = credentials_exp->user_login_id;
    sign_up_request_value.raw_password = credentials_exp->raw_password;
    return sign_up_request_value;
}

std::expected<auth_dto::credentials, http_error>
auth_request_parser::parse_credentials(const boost::json::object& json){
    const auto user_login_id_opt = json_field_util::get_non_empty_string_field(
        json,
        "user_login_id"
    );
    if(!user_login_id_opt){
        return std::unexpected(request_error::make_missing_field_error("user_login_id"));
    }

    const auto raw_password_opt = json_field_util::get_non_empty_string_field(
        json,
        "raw_password"
    );
    if(!raw_password_opt){
        return std::unexpected(request_error::make_missing_field_error("raw_password"));
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

    auth_dto::credentials credentials_value;
    credentials_value.user_login_id = std::string{*user_login_id_opt};
    credentials_value.raw_password = std::string{*raw_password_opt};
    return credentials_value;
}
