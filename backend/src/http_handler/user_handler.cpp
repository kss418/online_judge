#include "http_handler/user_handler.hpp"

#include "db_service/auth_service.hpp"
#include "common/permission_util.hpp"
#include "http_core/http_util.hpp"
#include "http_core/json_util.hpp"

user_handler::response_type user_handler::get_me(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto handle_authenticated =
        [&](const auth_dto::identity& auth_identity_value) -> response_type {
            return http_response_util::create_json(
                request,
                boost::beast::http::status::ok,
                json_util::make_user_me_object(auth_identity_value)
            );
        };

    return http_util::with_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

user_handler::response_type user_handler::put_user_admin(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto update_permission_level_exp = auth_service::update_permission_level(
            db_connection_value,
            user_id,
            permission_util::ADMIN
        );
        if(!update_permission_level_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "update permission level",
                update_permission_level_exp.error()
            );
        }
        if(!update_permission_level_exp.value()){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::not_found,
                "user_not_found",
                "user not found"
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_user_permission_object(user_id, permission_util::ADMIN)
        );
    };

    return http_util::with_superadmin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

user_handler::response_type user_handler::put_user_regular(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto update_permission_level_exp = auth_service::update_permission_level(
            db_connection_value,
            user_id,
            permission_util::USER
        );
        if(!update_permission_level_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "update permission level",
                update_permission_level_exp.error()
            );
        }
        if(!update_permission_level_exp.value()){
            return http_response_util::create_error(
                request,
                boost::beast::http::status::not_found,
                "user_not_found",
                "user not found"
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_user_permission_object(user_id, permission_util::USER)
        );
    };

    return http_util::with_superadmin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}

user_handler::response_type user_handler::get_user_list(
    const request_type& request,
    db_connection& db_connection_value
){
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto user_list_exp = auth_service::get_user_list(db_connection_value);
        if(!user_list_exp){
            return http_response_util::create_4xx_or_500(
                request,
                "get user list",
                user_list_exp.error()
            );
        }

        return http_response_util::create_json(
            request,
            boost::beast::http::status::ok,
            json_util::make_user_list_object(*user_list_exp)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}
