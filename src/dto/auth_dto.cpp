#include "dto/auth_dto.hpp"

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
