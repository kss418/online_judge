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
    using response_type = system_query_handler::response_type;

    template <typename command_type>
    using command_expected = std::expected<command_type, response_type>;

    auto make_get_health_spec(){
        return http_endpoint::make_json_spec(
            http_endpoint::make_no_input_guard(),
            [](system_query_handler::context_type&,
                const http_endpoint::no_input&) {
                return common_json_serializer::make_message_object("ok");
            },
            http_endpoint::identity_serializer{}
        );
    }

    auto make_get_supported_languages_spec(){
        return http_endpoint::make_json_spec(
            http_endpoint::make_no_input_guard(),
            [](system_query_handler::context_type&,
                const http_endpoint::no_input&) {
                return system_json_serializer::make_supported_language_list_object(
                    language_util::list_supported_languages()
                );
            },
            http_endpoint::identity_serializer{}
        );
    }

    auto make_get_status_spec(){
        return http_endpoint::make_guarded_json_spec(
            [](const http_guard::guard_context& context,
                const auth_dto::identity&)
                -> command_expected<get_system_status_query::command> {
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
            http_endpoint::make_db_execute(get_system_status_query::execute),
            system_json_serializer::make_status_object,
            auth_guard::make_admin_guard()
        );
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
