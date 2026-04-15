#include "http_handler/user_profile_query_handler.hpp"

#include "db_service/user_service.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "serializer/user_json_serializer.hpp"

#include <string>

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

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
                -> std::expected<std::int64_t, response_type> {
                return user_id;
            },
            http_endpoint::make_db_execute(user_service::get_summary),
            user_json_serializer::make_summary_object
        );
    }

    auto make_get_user_summary_by_login_id_spec(std::string_view user_login_id){
        return http_endpoint::make_guarded_json_spec(
            [user_login_id](const http_guard::guard_context&)
                -> std::expected<std::string, response_type> {
                return std::string{user_login_id};
            },
            http_endpoint::make_db_execute(
                user_service::get_summary_by_login_id
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
