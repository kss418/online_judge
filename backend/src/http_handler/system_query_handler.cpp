#include "http_handler/system_query_handler.hpp"

#include "application/get_system_status_query.hpp"

#include "common/language_util.hpp"
#include "http_core/http_response_util.hpp"
#include "http_core/http_runtime_status_provider.hpp"
#include "http_endpoint/endpoint.hpp"
#include "http_guard/auth_guard.hpp"
#include "serializer/common_json_serializer.hpp"
#include "serializer/system_json_serializer.hpp"

namespace{
    auto make_get_health_spec(){
        return http_endpoint::endpoint_spec{
            .parse = http_endpoint::make_no_input_guard(),
            .execute = [](system_query_handler::context_type&,
                const http_endpoint::no_input&) {
                return common_json_serializer::make_message_object("ok");
            },
            .serialize = http_endpoint::identity_serializer{},
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_supported_languages_spec(){
        return http_endpoint::endpoint_spec{
            .parse = http_endpoint::make_no_input_guard(),
            .execute = [](system_query_handler::context_type&,
                const http_endpoint::no_input&) {
                return system_json_serializer::make_supported_language_list_object(
                    language_util::list_supported_languages()
                );
            },
            .serialize = http_endpoint::identity_serializer{},
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }

    auto make_get_status_guard(){
        return http_guard::make_composite_guard(
            [](const http_guard::guard_context& context,
                const auth_dto::identity&)
                -> std::expected<
                    get_system_status_query::command,
                    system_query_handler::response_type
                > {
                if(!context.request_context_ref().has_http_runtime_status_provider()){
                    return std::unexpected(
                        http_response_util::create_internal_server_error(
                            context.request(),
                            "system_status",
                            "http runtime status provider is unavailable"
                        )
                    );
                }

                const auto& provider =
                    context.request_context_ref().http_runtime_status_provider_ref();
                return get_system_status_query::command{
                    .http_runtime_snapshot = provider.snapshot(),
                    .judge_heartbeat_stale_after = provider.judge_heartbeat_stale_after()
                };
            },
            auth_guard::make_admin_guard()
        );
    }

    auto make_get_status_spec(){
        return http_endpoint::endpoint_spec{
            .parse = make_get_status_guard(),
            .execute = [](system_query_handler::context_type& context,
                const get_system_status_query::command& command_value) {
                return get_system_status_query::execute(
                    context.db_connection_ref(),
                    command_value
                );
            },
            .serialize = system_json_serializer::make_status_object,
            .error_response = http_endpoint::default_error_response_factory{}
        };
    }
}

system_query_handler::response_type system_query_handler::get_health(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_health_spec());
}

system_query_handler::response_type system_query_handler::get_supported_languages(
    context_type& context
){
    return http_endpoint::run_json(
        context,
        make_get_supported_languages_spec()
    );
}

system_query_handler::response_type system_query_handler::get_status(
    context_type& context
){
    return http_endpoint::run_json(context, make_get_status_spec());
}
