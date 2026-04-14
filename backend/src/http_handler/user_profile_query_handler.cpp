#include "http_handler/user_profile_query_handler.hpp"

#include "application/get_user_summary_by_login_id_query.hpp"
#include "application/get_user_summary_query.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "serializer/user_json_serializer.hpp"

#include <string>

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_me_spec(){
        return http_endpoint::make_json_spec(
            auth_guard::make_auth_guard(),
            [](context_type&, const auth_dto::identity& auth_identity_value) {
                return auth_identity_value;
            },
            user_json_serializer::make_me_object
        );
    }

    auto make_get_user_summary_spec(std::int64_t user_id){
        return http_endpoint::make_guarded_json_spec(
            [user_id](const http_guard::guard_context&)
                -> command_expected<get_user_summary_query::command> {
                return get_user_summary_query::command{.user_id = user_id};
            },
            http_endpoint::make_db_execute(get_user_summary_query::execute),
            user_json_serializer::make_summary_object
        );
    }

    auto make_get_user_summary_by_login_id_spec(std::string_view user_login_id){
        return http_endpoint::make_guarded_json_spec(
            [user_login_id](const http_guard::guard_context&)
                -> command_expected<get_user_summary_by_login_id_query::command> {
                return get_user_summary_by_login_id_query::command{
                    .user_login_id = std::string{user_login_id}
                };
            },
            http_endpoint::make_db_execute(
                get_user_summary_by_login_id_query::execute
            ),
            user_json_serializer::make_summary_object
        );
    }
}

response_type user_query_handler::get_me(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_me_spec());
}

response_type user_query_handler::get_user_summary(
    context_type& context,
    std::int64_t user_id
){
    return http_endpoint::run_json(context, make_get_user_summary_spec(user_id));
}

response_type user_query_handler::get_user_summary_by_login_id(
    context_type& context,
    std::string_view user_login_id
){
    return http_endpoint::run_json(
        context,
        make_get_user_summary_by_login_id_spec(user_login_id)
    );
}
