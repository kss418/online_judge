#include "http_server/http_handler.hpp"

#include <pqxx/pqxx>

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include <boost/json.hpp>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/version.hpp>
#include <boost/system/error_code.hpp>

std::expected<http_handler, error_code> http_handler::create(
    db_connection db_connection
){
    if(!db_connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return http_handler(std::move(db_connection));
}

http_handler::http_handler(db_connection db_connection) :
    db_connection_(std::move(db_connection)){}

http_handler::response_type http_handler::create_text_response(
    const request_type& request, boost::beast::http::status status, std::string body
){
    response_type response{status, request.version()};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "text/plain; charset=utf-8");
    response.keep_alive(request.keep_alive());
    response.body() = std::move(body);
    response.prepare_payload();
    return response;
}

std::optional<http_handler::route_handler> http_handler::find_route_handler(
    boost::beast::http::verb method,
    std::string_view path
){
    for(const auto& route_definition_value : routes_){
        if(route_definition_value.method == method && route_definition_value.path == path){
            return route_definition_value.handler;
        }
    }

    return std::nullopt;
}

bool http_handler::has_route_path(std::string_view path){
    for(const auto& route_definition_value : routes_){
        if(route_definition_value.path == path){
            return true;
        }
    }

    return false;
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
    const auto* user_id_value = request_object.if_contains("user_id");
    const auto* problem_id_value = request_object.if_contains("problem_id");
    const auto* language_value = request_object.if_contains("language");
    const auto* source_code_value = request_object.if_contains("source_code");

    const bool has_valid_user_id = user_id_value && user_id_value->is_int64();
    const bool has_valid_problem_id = problem_id_value && problem_id_value->is_int64();
    const bool has_valid_language = language_value && language_value->is_string();
    const bool has_valid_source_code = source_code_value && source_code_value->is_string();

    if(
        !has_valid_user_id ||
        !has_valid_problem_id ||
        !has_valid_language ||
        !has_valid_source_code ||
        language_value->as_string().empty() ||
        source_code_value->as_string().empty()
    ){
        return create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: user_id, problem_id, language, source_code\n"
        );
    }

    std::expected<std::int64_t, error_code> submission_id_exp =
        std::unexpected(error_code::create(errno_error::unknown_error));
    try{
        pqxx::work transaction(db_connection_.connection());
        submission_id_exp = submission_util::create_submission(
            transaction,
            user_id_value->as_int64(),
            problem_id_value->as_int64(),
            std::string(language_value->as_string()),
            std::string(source_code_value->as_string())
        );
        if(submission_id_exp){
            transaction.commit();
        }
    }
    catch(const std::exception& exception){
        submission_id_exp = std::unexpected(error_code::map_psql_error_code(exception));
    }
    
    if(!submission_id_exp){
        const auto code = submission_id_exp.error();
        const bool is_invalid_argument_error = code.type_ == error_type::errno_type &&
            static_cast<errno_error>(code.code_) == errno_error::invalid_argument;
        const bool is_constraint_error = code.type_ == error_type::psql_type && (
            static_cast<psql_error>(code.code_) == psql_error::foreign_key_violation ||
            static_cast<psql_error>(code.code_) == psql_error::check_violation ||
            static_cast<psql_error>(code.code_) == psql_error::not_null_violation
        );
        const auto status = (is_invalid_argument_error || is_constraint_error)
            ? boost::beast::http::status::bad_request
            : boost::beast::http::status::internal_server_error;
        return create_text_response(
            request,
            status,
            "failed to create submission: " + to_string(code) + "\n"
        );
    }

    boost::json::object response_object;
    response_object["submission_id"] = *submission_id_exp;
    response_object["status"] = to_string(submission_status::queued);
    const std::string response_body = boost::json::serialize(response_object) + "\n";

    auto response = create_text_response(request, boost::beast::http::status::accepted, response_body);
    response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    return response;
}

http_handler::response_type http_handler::handle(const request_type& request){
    const std::string_view path{
        request.target().data(),
        request.target().size()
    };
    
    const auto route_handler_exp = find_route_handler(request.method(), path);
    if(route_handler_exp.has_value()){
        return std::invoke(route_handler_exp.value(), *this, request);
    }

    if(has_route_path(path)){
        return create_text_response(
            request, boost::beast::http::status::method_not_allowed, "method not allowed\n"
        );
    }

    return create_text_response(request, boost::beast::http::status::not_found, "not found\n");
}
