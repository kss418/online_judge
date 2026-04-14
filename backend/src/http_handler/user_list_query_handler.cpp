#include "http_handler/user_list_query_handler.hpp"

#include "application/get_public_user_list_query.hpp"
#include "application/get_user_list_query.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "http_guard/request_parse_guard.hpp"
#include "request_parser/user_request_parser.hpp"
#include "serializer/user_json_serializer.hpp"

namespace{
    using context_type = request_context;
    using response_type = request_context::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_public_user_list_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const user_dto::list_filter& filter_value)
                -> command_expected<get_public_user_list_query::command> {
                return get_public_user_list_query::command{
                    .filter_value = filter_value
                };
            },
            http_endpoint::make_db_execute(get_public_user_list_query::execute),
            user_json_serializer::make_public_list_object,
            request_parse_guard::make_query_guard<user_dto::list_filter>(
                user_request_parser::parse_list_filter
            )
        );
    }

    auto make_get_user_list_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context&,
                const auth_dto::identity&)
                -> command_expected<get_user_list_query::command> {
                return get_user_list_query::command{};
            },
            http_endpoint::make_db_execute(get_user_list_query::execute),
            user_json_serializer::make_list_object,
            auth_guard::make_admin_guard()
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
