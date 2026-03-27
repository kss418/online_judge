#include "http_handler/system_handler.hpp"
#include "common/language_util.hpp"
#include "http_core/json_util.hpp"
#include "http_core/http_response_util.hpp"

system_handler::response_type system_handler::handle_health_get(const request_type& request){
    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_message_object("ok")
    );
}

system_handler::response_type system_handler::handle_supported_languages_get(
    const request_type& request
){
    return http_response_util::create_json(
        request,
        boost::beast::http::status::ok,
        json_util::make_supported_language_list_object(
            language_util::list_supported_languages()
        )
    );
}
