#include "http_handler/user_command_handler.hpp"

#include "application/user_command_action.hpp"
#include "common/permission_util.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/user_request_parser.hpp"
#include "serializer/user_json_serializer.hpp"

namespace{
    auto make_put_user_admin_guard(std::int64_t user_id){
        return http_guard::make_composite_guard(
            [user_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    update_user_permission_action::command,
                    user_command_handler::response_type
                > {
                return update_user_permission_action::command{
                    .user_id = user_id,
                    .permission_level = permission_util::ADMIN
                };
            },
            auth_guard::make_superadmin_guard()
        );
    }

    auto make_put_user_admin_spec(std::int64_t user_id){
        return http_endpoint::endpoint_spec{
            .parse = make_put_user_admin_guard(user_id),
            .execute = [](user_command_handler::context_type& context,
                const update_user_permission_action::command& command_value) {
                return update_user_permission_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [user_id]() {
                return user_json_serializer::make_permission_object(
                    user_id,
                    permission_util::ADMIN
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_put_user_regular_guard(std::int64_t user_id){
        return http_guard::make_composite_guard(
            [user_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    update_user_permission_action::command,
                    user_command_handler::response_type
                > {
                return update_user_permission_action::command{
                    .user_id = user_id,
                    .permission_level = permission_util::USER
                };
            },
            auth_guard::make_superadmin_guard()
        );
    }

    auto make_put_user_regular_spec(std::int64_t user_id){
        return http_endpoint::endpoint_spec{
            .parse = make_put_user_regular_guard(user_id),
            .execute = [](user_command_handler::context_type& context,
                const update_user_permission_action::command& command_value) {
                return update_user_permission_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = [user_id]() {
                return user_json_serializer::make_permission_object(
                    user_id,
                    permission_util::USER
                );
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_post_user_submission_ban_guard(std::int64_t user_id){
        return http_guard::make_composite_guard(
            [user_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const user_dto::submission_ban_request& submission_ban_request)
                -> std::expected<
                    create_user_submission_ban_action::command,
                    user_command_handler::response_type
                > {
                return create_user_submission_ban_action::command{
                    .user_id = user_id,
                    .duration_minutes = submission_ban_request.duration_minutes
                };
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<user_dto::submission_ban_request>(
                user_request_parser::parse_submission_ban_request
            )
        );
    }

    auto make_post_user_submission_ban_spec(std::int64_t user_id){
        return http_endpoint::endpoint_spec{
            .parse = make_post_user_submission_ban_guard(user_id),
            .execute = [](user_command_handler::context_type& context,
                const create_user_submission_ban_action::command& command_value) {
                return create_user_submission_ban_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = user_json_serializer::make_submission_ban_object,
            .error_response = http_endpoint::default_error_response_factory{},
            .success_status = boost::beast::http::status::created
        };
    }

    auto make_delete_user_submission_ban_guard(std::int64_t user_id){
        return http_guard::make_composite_guard(
            [user_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<
                    clear_user_submission_ban_action::command,
                    user_command_handler::response_type
                > {
                return clear_user_submission_ban_action::command{
                    .user_id = user_id
                };
            },
            auth_guard::make_admin_guard()
        );
    }

    auto make_delete_user_submission_ban_spec(std::int64_t user_id){
        return http_endpoint::endpoint_spec{
            .parse = make_delete_user_submission_ban_guard(user_id),
            .execute = [](user_command_handler::context_type& context,
                const clear_user_submission_ban_action::command& command_value) {
                return clear_user_submission_ban_action::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = []() -> std::string_view {
                return "user submission ban cleared";
            },
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }
}

user_command_handler::response_type user_command_handler::put_user_admin(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_put_user_admin_spec(user_id)
    );
}

user_command_handler::response_type user_command_handler::put_user_regular(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_put_user_regular_spec(user_id)
    );
}

user_command_handler::response_type user_command_handler::post_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_post_user_submission_ban_spec(user_id)
    );
}

user_command_handler::response_type user_command_handler::delete_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_message(
        context,
        make_delete_user_submission_ban_spec(user_id)
    );
}
