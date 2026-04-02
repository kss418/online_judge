#include "http_core/http_response_util.hpp"

#include "common/logger.hpp"
#include "error/http_error.hpp"
#include "serializer/common_json_serializer.hpp"

#include <utility>

#include <boost/beast/http/field.hpp>
#include <boost/beast/version.hpp>

namespace{
    http_response_util::response_type create_text_response(
        const http_response_util::request_type& request,
        boost::beast::http::status status,
        std::string body
    ){
        http_response_util::response_type response{status, request.version()};
        response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        response.set(
            boost::beast::http::field::content_type,
            "text/plain; charset=utf-8"
        );
        response.keep_alive(request.keep_alive());
        response.body() = std::move(body);
        response.prepare_payload();
        return response;
    }

    http_response_util::response_type create_mapped_error_response(
        const http_response_util::request_type& request,
        const service_error& code
    ){
        return http_response_util::create_error(
            request,
            http_error::from_service_error(code)
        );
    }
}

http_response_util::response_type http_response_util::create_json(
    const request_type& request,
    boost::beast::http::status status,
    const boost::json::value& response_value
){
    auto response = create_text_response(
        request,
        status,
        boost::json::serialize(response_value) + "\n"
    );
    response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    return response;
}

http_response_util::response_type http_response_util::create_message(
    const request_type& request,
    boost::beast::http::status status,
    std::string_view message
){
    return create_json(
        request,
        status,
        common_json_serializer::make_message_object(message)
    );
}

http_response_util::response_type http_response_util::create_error(
    const request_type& request,
    const http_error& error
){
    auto response = create_json(
        request,
        error.status(),
        common_json_serializer::make_error_object(
            error.code_string(),
            error.message,
            error.field_opt
        )
    );
    if(error.requires_bearer_auth()){
        response.set(boost::beast::http::field::www_authenticate, "Bearer");
    }
    return response;
}

http_response_util::response_type http_response_util::create_4xx_or_500(
    const request_type& request,
    const service_error& code
){
    return create_mapped_error_response(request, code);
}

http_response_util::response_type http_response_util::create_internal_server_error(
    const request_type& request,
    std::string_view context,
    std::string_view detail
){
    auto log_record = logger::cerr().log("http_internal_server_error");
    log_record
        .field("context", context)
        .field(
            "method",
            std::string_view{
                request.method_string().data(),
                request.method_string().size()
            }
        )
        .field(
            "target",
            std::string_view{
                request.target().data(),
                request.target().size()
            }
        );
    if(!detail.empty()){
        log_record.field("detail", detail);
    }

    return create_error(request, http_error{http_error_code::internal_server_error});
}

http_response_util::response_type http_response_util::create_service_unavailable(
    const request_type& request,
    std::string_view message
){
    if(message.empty()){
        return create_error(request, http_error{http_error_code::service_unavailable});
    }

    return create_error(
        request,
        http_error{http_error_code::service_unavailable, std::string{message}}
    );
}

http_response_util::response_type http_response_util::create_method_not_allowed(
    const request_type& request
){
    return create_error(request, http_error{http_error_code::method_not_allowed});
}

http_response_util::response_type http_response_util::create_not_found(
    const request_type& request
){
    return create_error(request, http_error{http_error_code::not_found});
}
