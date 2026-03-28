#include "http_handler/system_handler.hpp"
#include "common/language_util.hpp"
#include "http_core/http_response_util.hpp"
#include "serializer/common_json_serializer.hpp"
#include "serializer/system_json_serializer.hpp"

system_handler::response_type system_handler::handle_health_get(const request_type& request){
    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        common_json_serializer::make_message_object("ok")
    );
}

system_handler::response_type system_handler::handle_supported_languages_get(
    const request_type& request
){
    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        system_json_serializer::make_supported_language_list_object(
            language_util::list_supported_languages()
        )
    );
}
