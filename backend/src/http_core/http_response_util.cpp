#include "http_core/http_response_util.hpp"

#include "common/string_util.hpp"
#include "serializer/common_json_serializer.hpp"

#include <utility>

#include <boost/beast/http/field.hpp>
#include <boost/beast/version.hpp>

namespace{
    struct mapped_http_error{
        boost::beast::http::status status;
        std::string_view code;
    };

    std::optional<mapped_http_error> map_http_error(const error_code& code){
        if(code == http_error::validation_error){
            return mapped_http_error{
                .status = boost::beast::http::status::bad_request,
                .code = "validation_error"
            };
        }
        if(code == http_error::unauthorized){
            return mapped_http_error{
                .status = boost::beast::http::status::unauthorized,
                .code = "unauthorized"
            };
        }
        if(code == http_error::forbidden){
            return mapped_http_error{
                .status = boost::beast::http::status::forbidden,
                .code = "forbidden"
            };
        }
        if(code == http_error::not_found){
            return mapped_http_error{
                .status = boost::beast::http::status::not_found,
                .code = "not_found"
            };
        }
        if(code == http_error::conflict || code == psql_error::unique_violation){
            return mapped_http_error{
                .status = boost::beast::http::status::conflict,
                .code = "conflict"
            };
        }

        return std::nullopt;
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
    const error_code& code
){
    boost::beast::http::status status = boost::beast::http::status::internal_server_error;
    std::string_view error_code_text = "internal_server_error";
    if(const auto mapped_error_opt = map_http_error(code)){
        status = mapped_error_opt->status;
        error_code_text = mapped_error_opt->code;
    }
    else if(code.is_bad_request_error()){
        status = boost::beast::http::status::bad_request;
        error_code_text = "bad_request";
    }

    return create_error(
        request,
        status,
        error_code_text,
        "failed to " + std::string{action} + ": " + to_string(code)
    );
}

http_response_util::response_type http_response_util::create_404_or_500(
    const request_type& request,
    std::string_view action,
    const error_code& code
){
    if(const auto mapped_error_opt = map_http_error(code)){
        return create_error(
            request,
            mapped_error_opt->status,
            mapped_error_opt->code,
            "failed to " + std::string{action} + ": " + to_string(code)
        );
    }

    const auto status = code.is_bad_request_error()
        ? boost::beast::http::status::not_found
        : boost::beast::http::status::internal_server_error;
    const auto error_code_text = code.is_bad_request_error()
        ? "not_found"
        : "internal_server_error";

    return create_error(
        request,
        status,
        error_code_text,
        "failed to " + std::string{action} + ": " + to_string(code)
    );
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
