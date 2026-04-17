#include "http_handler/user_profile_query_handler.hpp"

#include "db_service/user_service.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/optional_auth_spec_helper.hpp"
#include "http_handler/path_value_spec_helper.hpp"
#include "http_handler/user_path_spec_helper.hpp"
#include "serializer/user_json_serializer.hpp"

#include <string>

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    auto make_get_me_spec(){
        return http_handler_spec::make_auth_identity_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity& auth_identity_value)
                -> std::expected<auth_dto::identity, response_type> {
                return auth_identity_value;
            },
            http_endpoint::identity_serializer{},
            user_json_serializer::make_me_object
        );
    }

    auto make_get_user_summary_spec(std::int64_t user_id){
        return http_handler_spec::make_existing_user_json_spec(
            user_id,
            user_service::get_summary,
            user_json_serializer::make_summary_object
        );
    }

    auto make_get_user_summary_by_login_id_spec(std::string_view user_login_id){
        return http_handler_spec::make_single_path_value_json_spec(
            user_login_id,
            [](const http_guard::guard_context&, std::string_view user_login_id)
                -> std::expected<std::string, response_type> {
                return std::string{user_login_id};
            },
            user_service::get_summary_by_login_id,
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
