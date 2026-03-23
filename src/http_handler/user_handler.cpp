#include "http_handler/user_handler.hpp"

#include "db_service/auth_service.hpp"
#include "http_server/http_util.hpp"
#include "http_server/json_util.hpp"

user_handler::response_type user_handler::put_user_admin(
    const request_type& request,
    db_connection& db_connection_value,
    std::int64_t user_id
){
    const auto handle_authenticated = [&](const auth_dto::identity&) -> response_type {
        const auto update_admin_status_exp = auth_service::update_admin_status(
            db_connection_value,
            user_id,
            true
        );
        if(!update_admin_status_exp){
            return http_response_util::create_400_or_500(
                request,
                "update admin status",
                update_admin_status_exp.error()
            );
        }
        if(!update_admin_status_exp.value()){
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
            json_util::make_user_admin_object(user_id, true)
        );
    };

    return http_util::with_admin_auth_bearer(
        request,
        db_connection_value,
        handle_authenticated
    );
}
