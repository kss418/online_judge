#include "http_server/http_handler.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

#include <boost/json.hpp>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/version.hpp>
#include <boost/system/error_code.hpp>

http_handler::response_type http_handler::create_text_response(
    const request_type& request, boost::beast::http::status status, std::string body
){
    http_handler::response_type response{status, request.version()};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "text/plain; charset=utf-8");
    response.keep_alive(request.keep_alive());
    response.body() = std::move(body);
    response.prepare_payload();
    return response;
}

http_handler::response_type http_handler::handle_health_get(const request_type& request){
    return create_text_response(request, boost::beast::http::status::ok, "ok\n");
}

http_handler::response_type http_handler::handle_submission(const request_type& request){
    if(request.body().empty()){
        return create_text_response(
            request, boost::beast::http::status::bad_request, "request body is empty\n"
        );
    }

    boost::system::error_code ec;
    auto request_value = boost::json::parse(request.body(), ec);
    if(ec || !request_value.is_object()){
        return create_text_response(request, boost::beast::http::status::bad_request, "invalid json\n");
    }

    const auto& request_object = request_value.as_object();
    const auto* problem_id_value = request_object.if_contains("problem_id");
    const auto* language_value = request_object.if_contains("language");
    const auto* source_code_value = request_object.if_contains("source_code");

    const bool has_valid_problem_id = problem_id_value && problem_id_value->is_int64();
    const bool has_valid_language = language_value && language_value->is_string();
    const bool has_valid_source_code = source_code_value && source_code_value->is_string();

    if(
        !has_valid_problem_id ||
        !has_valid_language ||
        !has_valid_source_code ||
        language_value->as_string().empty() ||
        source_code_value->as_string().empty()
    ){
        return create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: problem_id, language, source_code\n"
        );
    }

    const std::int64_t problem_id = problem_id_value->as_int64();
    static std::atomic<std::uint64_t> next_submission_id{12345};
    const std::uint64_t created_submission_id = next_submission_id.fetch_add(1, std::memory_order_relaxed);

    boost::json::object response_object;
    response_object["submission_id"] = created_submission_id;
    response_object["status"] = "queued";
    const std::string response_body = boost::json::serialize(response_object) + "\n";

    auto response = create_text_response(request, boost::beast::http::status::accepted, response_body);
    response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    return response;
}

http_handler::response_type http_handler::handle(const request_type& request){
    for(const auto& entry : routes){
        if(entry.method_ == request.method() && entry.target_ == request.target()){
            return entry.handler_(request);
        }
    }

    for(const auto& entry : routes){
        if(entry.target_ == request.target()){
            return create_text_response(
                request, boost::beast::http::status::method_not_allowed, "method not allowed\n"
            );
        }
    }

    return create_text_response(request, boost::beast::http::status::not_found, "not found\n");
}
