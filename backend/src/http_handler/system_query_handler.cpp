#include "http_handler/system_query_handler.hpp"
#include "common/language_util.hpp"
#include "db_service/system_service.hpp"
#include "http_core/http_adapter.hpp"
#include "http_core/http_response_util.hpp"
#include "http_core/http_runtime_status_provider.hpp"
#include "http_guard/auth_guard.hpp"
#include "serializer/common_json_serializer.hpp"
#include "serializer/system_json_serializer.hpp"

system_query_handler::response_type system_query_handler::get_health(
    context_type& context
){
    return http_adapter::json(
        context.request,
        common_json_serializer::make_message_object("ok")
    );
}

system_query_handler::response_type system_query_handler::get_supported_languages(
    context_type& context
){
    return http_adapter::json(
        context.request,
        system_json_serializer::make_supported_language_list_object(
            language_util::list_supported_languages()
        )
    );
}

system_query_handler::response_type system_query_handler::get_status(
    context_type& context
){
    return http_guard::run_or_respond(
        context,
        [](context_type& context_value,
            const auth_dto::identity&) -> response_type {
            if(!context_value.has_http_runtime_status_provider()){
                return http_response_util::create_internal_server_error(
                    context_value.request,
                    "system_status",
                    "http runtime status provider is unavailable"
                );
            }

            const auto system_status_exp = system_service::get_status(
                context_value.db_connection_ref(),
                context_value.http_runtime_status_provider_ref().snapshot(),
                context_value.http_runtime_status_provider_ref()
                    .judge_heartbeat_stale_after()
            );
            return http_adapter::json(
                context_value.request,
                std::move(system_status_exp),
                system_json_serializer::make_status_object
            );
        },
        auth_guard::make_admin_guard()
    );
}
