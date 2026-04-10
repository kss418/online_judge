#include "http_handler/system_handler.hpp"
#include "common/language_util.hpp"
#include "http_core/http_response_util.hpp"
#include "serializer/common_json_serializer.hpp"
#include "serializer/system_json_serializer.hpp"

system_handler::response_type system_handler::get_health(context_type& context){
    return http_response_util::create_json(
        context.request,
        boost::beast::http::status::ok,
        common_json_serializer::make_message_object("ok")
    );
}

system_handler::response_type system_handler::get_supported_languages(
    context_type& context
){
    return http_response_util::create_json(
        context.request,
        boost::beast::http::status::ok,
        system_json_serializer::make_supported_language_list_object(
            language_util::list_supported_languages()
        )
    );
}
