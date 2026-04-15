#include "http_handler/user_submission_ban_query_handler.hpp"

#include "db_service/user_service.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/handler_spec_helper.hpp"
#include "http_guard/auth_guard.hpp"
#include "serializer/user_json_serializer.hpp"

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    auto make_get_me_submission_ban_spec(){
        return http_handler_spec::make_auth_identity_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<std::int64_t, response_type> {
                return auth_identity_value.user_id;
            },
            user_service::get_submission_ban_status,
            user_json_serializer::make_submission_ban_status_object
        );
    }

    auto make_get_user_submission_ban_spec(std::int64_t user_id){
        return http_handler_spec::make_single_path_value_json_spec(
            user_id,
            [](const http_guard::guard_context&,
                std::int64_t user_id,
                const auth_dto::identity&)
                -> std::expected<std::int64_t, response_type> {
                return user_id;
            },
            user_service::get_submission_ban_status,
            user_json_serializer::make_submission_ban_status_object,
            auth_guard::make_admin_guard()
        );
    }
}

response_type user_query_handler::get_me_submission_ban(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_submission_ban_spec());
}

response_type user_query_handler::get_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_submission_ban_spec(user_id)
    );
}
