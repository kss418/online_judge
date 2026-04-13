#include "http_handler/user_command_handler.hpp"

#include "application/user_command_action.hpp"
#include "common/permission_util.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/user_request_parser.hpp"
#include "serializer/user_json_serializer.hpp"

#include <string_view>

namespace{
    using response_type = user_command_handler::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_update_user_permission_spec(
        std::int64_t user_id,
        std::int32_t permission_level
    ){
        return http_endpoint::make_guarded_json_spec(
            [user_id, permission_level](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<update_user_permission_action::command> {
                return update_user_permission_action::command{
                    .user_id = user_id,
                    .permission_level = permission_level
                };
            },
            http_endpoint::make_db_execute(update_user_permission_action::execute),
            [user_id, permission_level]() {
                return user_json_serializer::make_permission_object(
                    user_id,
                    permission_level
                );
            },
            auth_guard::make_superadmin_guard()
        );
    }

    auto make_put_user_admin_spec(std::int64_t user_id){
        return make_update_user_permission_spec(user_id, permission_util::ADMIN);
    }

    auto make_put_user_regular_spec(std::int64_t user_id){
        return make_update_user_permission_spec(user_id, permission_util::USER);
    }

    auto make_post_user_submission_ban_spec(std::int64_t user_id){
        return http_endpoint::make_guarded_json_spec(
            [user_id](const http_guard::guard_context&,
                const auth_dto::identity&,
                const user_dto::submission_ban_request& submission_ban_request)
                -> command_expected<create_user_submission_ban_action::command> {
                return create_user_submission_ban_action::command{
                    .user_id = user_id,
                    .duration_minutes = submission_ban_request.duration_minutes
                };
            },
            http_endpoint::make_db_execute(create_user_submission_ban_action::execute),
            user_json_serializer::make_submission_ban_object,
            http_endpoint::spec_options{
                .success_status = boost::beast::http::status::created
            },
            auth_guard::make_admin_guard(),
            request_parse_guard::make_json_guard<user_dto::submission_ban_request>(
                user_request_parser::parse_submission_ban_request
            )
        );
    }

    auto make_delete_user_submission_ban_spec(std::int64_t user_id){
        return http_endpoint::make_guarded_message_spec(
            [user_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<clear_user_submission_ban_action::command> {
                return clear_user_submission_ban_action::command{
                    .user_id = user_id
                };
            },
            http_endpoint::make_db_execute(clear_user_submission_ban_action::execute),
            []() -> std::string_view {
                return "user submission ban cleared";
            },
            auth_guard::make_admin_guard()
        );
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
