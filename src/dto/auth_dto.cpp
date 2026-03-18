#include "dto/auth_dto.hpp"

#include "http_server/http_util.hpp"

std::optional<auth_dto::credentials> auth_dto::make_credentials(
    const boost::json::object& request_object
){
    const auto user_login_id_opt = http_util::get_non_empty_string_field(
        request_object,
        "user_login_id"
    );
    const auto raw_password_opt = http_util::get_non_empty_string_field(
        request_object,
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
