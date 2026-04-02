#include "http_core/http_response_util.hpp"

#include "common/string_util.hpp"
#include "error/http_error.hpp"
#include "serializer/common_json_serializer.hpp"

#include <utility>

#include <boost/beast/http/field.hpp>
#include <boost/beast/version.hpp>

namespace{
    struct mapped_http_error{
        boost::beast::http::status status;
        std::string_view code;
    };

    mapped_http_error map_http_error(const service_error& code){
        const auto http_error_value = http_error::from_service(code);

        if(http_error_value == http_error::validation_error){
            return mapped_http_error{
                .status = boost::beast::http::status::bad_request,
                .code = "validation_error"
            };
        }
        if(http_error_value == http_error::unauthorized){
            return mapped_http_error{
                .status = boost::beast::http::status::unauthorized,
                .code = "unauthorized"
            };
        }
        if(http_error_value == http_error::forbidden){
            return mapped_http_error{
                .status = boost::beast::http::status::forbidden,
                .code = "forbidden"
            };
        }
        if(http_error_value == http_error::not_found){
            return mapped_http_error{
                .status = boost::beast::http::status::not_found,
                .code = "not_found"
            };
        }
        if(http_error_value == http_error::conflict){
            return mapped_http_error{
                .status = boost::beast::http::status::conflict,
                .code = "conflict"
            };
        }

        return mapped_http_error{
            .status = boost::beast::http::status::internal_server_error,
            .code = "internal_server_error"
        };
    }

    template <typename error_type>
    http_response_util::response_type create_mapped_error_response(
        const http_response_util::request_type& request,
        std::string_view action,
        const error_type& code,
        bool not_found_from_bad_request
    ){
        auto mapped_error = map_http_error(code);
        if(
            not_found_from_bad_request &&
            mapped_error.status == boost::beast::http::status::bad_request
        ){
            mapped_error.status = boost::beast::http::status::not_found;
            mapped_error.code = "not_found";
        }

        return http_response_util::create_error(
            request,
            mapped_error.status,
            mapped_error.code,
            "failed to " + std::string{action} + ": " + to_string(code)
        );
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
    auto response = create_bearer_error(
        request,
        "unauthorized",
        message
    );
    return response;
}

http_response_util::response_type http_response_util::create_method_not_allowed(
    const request_type& request
){
    return create_error(
        request,
        boost::beast::http::status::method_not_allowed,
        "method_not_allowed",
        "method not allowed"
    );
}

http_response_util::response_type http_response_util::create_not_found(
    const request_type& request
){
    return create_error(
        request,
        boost::beast::http::status::not_found,
        "not_found",
        "not found"
    );
}

http_response_util::response_type http_response_util::create_problem_not_found(
    const request_type& request
){
    return create_error(
        request,
        boost::beast::http::status::not_found,
        "problem_not_found",
        "problem not found"
    );
}

http_response_util::response_type http_response_util::create_user_not_found(
    const request_type& request
){
    return create_error(
        request,
        boost::beast::http::status::not_found,
        "user_not_found",
        "user not found"
    );
}
