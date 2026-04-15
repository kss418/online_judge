#include "http_handler/user_command_handler.hpp"

#include "common/permission_util.hpp"
#include "db_service/auth_service.hpp"
#include "db_service/user_service.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/handler_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "http_guard/user_guard.hpp"
#include "request_parser/user_request_parser.hpp"
#include "serializer/user_json_serializer.hpp"

#include <string_view>

namespace{
    using response_type = user_command_handler::response_type;

    struct update_user_permission_request{
        std::int64_t user_id = 0;
        std::int32_t permission_level = 0;
    };

    struct create_submission_ban_request{
        std::int64_t user_id = 0;
        std::int32_t duration_minutes = 0;
    };

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_update_user_permission_spec(
        std::int64_t user_id,
        std::int32_t permission_level
    ){
        return http_handler_spec::make_single_path_value_json_spec(
            user_id,
            [permission_level](const http_guard::guard_context&,
                std::int64_t user_id,
                const auth_dto::identity&)
                -> command_expected<update_user_permission_request> {
                return update_user_permission_request{
                    .user_id = user_id,
                    .permission_level = permission_level
                };
            },
            [](auto& db_connection, const update_user_permission_request& request) {
                return auth_service::update_permission_level(
                    db_connection,
                    request.user_id,
                    request.permission_level
                );
            },
            [user_id, permission_level]() {
                return user_json_serializer::make_permission_object(
                    user_id,
                    permission_level
                );
            },
            auth_guard::make_superadmin_guard(),
            user_guard::make_exists_guard(user_id)
        );
    }

    auto make_put_user_admin_spec(std::int64_t user_id){
        return make_update_user_permission_spec(user_id, permission_util::ADMIN);
    }

    auto make_put_user_regular_spec(std::int64_t user_id){
        return make_update_user_permission_spec(user_id, permission_util::USER);
    }

    auto make_post_user_submission_ban_spec(std::int64_t user_id){
        return http_handler_spec::make_single_path_value_json_spec(
            user_id,
            [](const http_guard::guard_context&,
                std::int64_t user_id,
                const auth_dto::identity&,
                const user_dto::submission_ban_request& submission_ban_request)
                -> command_expected<create_submission_ban_request> {
                return create_submission_ban_request{
                    .user_id = user_id,
                    .duration_minutes = submission_ban_request.duration_minutes
                };
            },
            [](auto& db_connection, const create_submission_ban_request& request) {
                return user_service::create_submission_ban(
                    db_connection,
                    request.user_id,
                    request.duration_minutes
                );
            },
            user_json_serializer::make_submission_ban_object,
            http_endpoint::spec_options{
                .success_status = boost::beast::http::status::created
            },
            auth_guard::make_admin_guard(),
            user_guard::make_exists_guard(user_id),
            request_parse_guard::make_json_guard<user_dto::submission_ban_request>(
                user_request_parser::parse_submission_ban_request
            )
        );
    }

    auto make_delete_user_submission_ban_spec(std::int64_t user_id){
        return http_handler_spec::make_single_path_value_message_spec(
            user_id,
            [](const http_guard::guard_context&,
                std::int64_t user_id,
                const auth_dto::identity&)
                -> command_expected<std::int64_t> {
                return user_id;
            },
            user_service::clear_submission_banned_until,
            []() -> std::string_view {
                return "user submission ban cleared";
            },
            auth_guard::make_admin_guard(),
            user_guard::make_exists_guard(user_id)
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
