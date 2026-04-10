#include "http_handler/user_command_handler.hpp"

#include "common/permission_util.hpp"
#include "db_service/auth_service.hpp"
#include "db_service/user_service.hpp"
#include "http_core/http_adapter.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_guard.hpp"
#include "request_parser/user_request_parser.hpp"
#include "serializer/user_json_serializer.hpp"

user_command_handler::response_type user_command_handler::put_user_admin(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&) -> response_type {
            const auto update_permission_level_exp = auth_service::update_permission_level(
                context.db_connection_ref(),
                user_id,
                permission_util::ADMIN
            );
            return http_adapter::json(
                context.request,
                std::move(update_permission_level_exp),
                [&] {
                    return user_json_serializer::make_permission_object(
                        user_id,
                        permission_util::ADMIN
                    );
                }
            );
        },
        auth_guard::make_superadmin_guard()
    );
}

user_command_handler::response_type user_command_handler::put_user_regular(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&) -> response_type {
            const auto update_permission_level_exp = auth_service::update_permission_level(
                context.db_connection_ref(),
                user_id,
                permission_util::USER
            );
            return http_adapter::json(
                context.request,
                std::move(update_permission_level_exp),
                [&] {
                    return user_json_serializer::make_permission_object(
                        user_id,
                        permission_util::USER
                    );
                }
            );
        },
        auth_guard::make_superadmin_guard()
    );
}

user_command_handler::response_type user_command_handler::post_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&,
            const user_dto::submission_ban_request& submission_ban_request) -> response_type {
            const auto create_submission_ban_exp = user_service::create_submission_ban(
                context.db_connection_ref(),
                user_id,
                submission_ban_request.duration_minutes
            );
            return http_adapter::json(
                context.request,
                std::move(create_submission_ban_exp),
                user_json_serializer::make_submission_ban_object,
                boost::beast::http::status::created
            );
        },
        auth_guard::make_admin_guard(),
        request_guard::make_json_guard<user_dto::submission_ban_request>(
            user_request_parser::parse_submission_ban_request
        )
    );
}

user_command_handler::response_type user_command_handler::delete_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_guard::run_or_respond(
        context,
        [&](const auth_dto::identity&) -> response_type {
            const auto clear_submission_banned_until_exp =
                user_service::clear_submission_banned_until(
                    context.db_connection_ref(),
                    user_id
                );
            return http_adapter::message(
                context.request,
                std::move(clear_submission_banned_until_exp),
                "user submission ban cleared"
            );
        },
        auth_guard::make_admin_guard()
    );
}
