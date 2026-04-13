#include "http_handler/user_query_handler.hpp"

#include "application/get_user_submission_ban_status_query.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "serializer/user_json_serializer.hpp"

namespace{
    using context_type = user_query_handler::context_type;
    using response_type = user_query_handler::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_me_submission_ban_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> command_expected<get_user_submission_ban_status_query::command> {
                return get_user_submission_ban_status_query::command{
                    .user_id = auth_identity_value.user_id
                };
            },
            http_endpoint::make_db_execute(
                get_user_submission_ban_status_query::execute
            ),
            user_json_serializer::make_submission_ban_status_object,
            auth_guard::make_auth_guard()
        );
    }

    auto make_get_user_submission_ban_spec(std::int64_t user_id){
        return http_endpoint::make_guarded_json_spec(
            [user_id](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<get_user_submission_ban_status_query::command> {
                return get_user_submission_ban_status_query::command{
                    .user_id = user_id
                };
            },
            http_endpoint::make_db_execute(
                get_user_submission_ban_status_query::execute
            ),
            user_json_serializer::make_submission_ban_status_object,
            auth_guard::make_admin_guard()
        );
    }
}

user_query_handler::response_type user_query_handler::get_me_submission_ban(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_submission_ban_spec());
}

user_query_handler::response_type user_query_handler::get_user_submission_ban(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_submission_ban_spec(user_id)
    );
}
