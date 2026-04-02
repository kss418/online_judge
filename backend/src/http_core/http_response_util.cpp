#include "http_core/http_response_util.hpp"

#include "common/string_util.hpp"
#include "error/http_error.hpp"
#include "serializer/common_json_serializer.hpp"

#include <utility>

#include <boost/beast/http/field.hpp>
#include <boost/beast/version.hpp>

namespace{
    http_error map_not_found_from_bad_request(http_error error){
        if(error.status() != boost::beast::http::status::bad_request){
            return error;
        }

        return http_error{
            http_error_code::not_found,
            std::move(error.message),
            std::move(error.field_opt)
        };
    }

    template <typename error_type>
    http_response_util::response_type create_mapped_error_response(
        const http_response_util::request_type& request,
        std::string_view action,
        const error_type& code,
        bool not_found_from_bad_request
    ){
        auto mapped_error = http_error::from_service_error(code);
        mapped_error.message = "failed to " + std::string{action} + ": " + to_string(code);
        if(not_found_from_bad_request){
            mapped_error = map_not_found_from_bad_request(std::move(mapped_error));
        }

        return http_response_util::create_error(request, mapped_error);
    }
}

http_response_util::response_type http_response_util::create_text(
    const request_type& request,
    boost::beast::http::status status,
    std::string body
){
    response_type response{status, request.version()};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "text/plain; charset=utf-8");
    response.keep_alive(request.keep_alive());
    response.body() = std::move(body);
    response.prepare_payload();
    return response;
}

http_response_util::response_type http_response_util::create_json(
    const request_type& request,
    boost::beast::http::status status,
    const boost::json::value& response_value
){
    auto response = create_text(
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
    boost::beast::http::status status,
    std::string_view code,
    std::string_view message,
    std::optional<std::string> field_opt
){
    return create_json(
        request,
        status,
        common_json_serializer::make_error_object(code, message, std::move(field_opt))
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

http_response_util::response_type http_response_util::create_bearer_error(
    const request_type& request,
    std::string_view code,
    std::string_view message
){
    auto response = create_error(
        request,
        boost::beast::http::status::unauthorized,
        code,
        message
    );
    response.set(boost::beast::http::field::www_authenticate, "Bearer");
    return response;
}

http_response_util::response_type http_response_util::create_4xx_or_500(
    const request_type& request,
    std::string_view action,
    const service_error& code
){
    return create_mapped_error_response(request, action, code, false);
}

http_response_util::response_type http_response_util::create_404_or_500(
    const request_type& request,
    std::string_view action,
    const service_error& code
){
    return create_mapped_error_response(request, action, code, true);
}

http_response_util::response_type http_response_util::create_bearer_unauthorized(
    const request_type& request,
    std::string body
){
    const auto message = string_util::trim_right_whitespace(body);
    return create_bearer_error(request, to_code_string(http_error_code::unauthorized), message);
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

http_response_util::response_type http_response_util::create_resource_not_found(
    const request_type& request,
    std::string_view resource_name
){
    const std::string resource_name_value{resource_name};
    return create_error(
        request,
        boost::beast::http::status::not_found,
        resource_name_value + "_not_found",
        resource_name_value + " not found"
    );
}

http_response_util::response_type http_response_util::create_problem_not_found(
    const request_type& request
){
    return create_error(request, http_error{http_error_code::problem_not_found});
}

http_response_util::response_type http_response_util::create_user_not_found(
    const request_type& request
){
    return create_error(request, http_error{http_error_code::user_not_found});
}

http_response_util::response_type http_response_util::create_testcase_not_found(
    const request_type& request
){
    return create_error(request, http_error{http_error_code::testcase_not_found});
}
