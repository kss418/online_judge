#include "http_handler/user_list_query_handler.hpp"

#include "db_service/auth_service.hpp"
#include "db_service/user_service.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_handler/optional_auth_spec_helper.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/user_request_parser.hpp"
#include "serializer/user_json_serializer.hpp"

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    auto make_get_public_user_list_spec(){
        return http_endpoint::make_json_spec(
            request_parse_guard::make_query_guard<user_dto::list_filter>(
                user_request_parser::parse_list_filter
            ),
            http_endpoint::make_db_execute(user_service::get_public_list),
            user_json_serializer::make_public_list_object
        );
    }

    auto make_get_user_list_spec(){
        return http_handler_spec::make_admin_identity_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> std::expected<http_endpoint::no_input, response_type> {
                return http_endpoint::no_input{};
            },
            [](request_context& context, const http_endpoint::no_input&) {
                return auth_service::get_user_list(context.db_connection_ref());
            },
            user_json_serializer::make_list_object
        );
    }
}

response_type user_query_handler::get_public_user_list(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_public_user_list_spec());
}

response_type user_query_handler::get_user_list(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_user_list_spec());
}
