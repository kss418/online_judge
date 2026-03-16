#include "http_server/http_handler.hpp"
#include "http_server/http_util.hpp"

#include "db/login_service.hpp"

#include <utility>

#include <boost/json.hpp>

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

http_handler::response_type http_handler::handle_health_get(const request_type& request){
    return http_util::create_text_response(
        request,
        boost::beast::http::status::ok,
        "ok\n"
    );
}

http_handler::response_type http_handler::handle_sign_up_post(const request_type& request){
    if(request.body().empty()){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "request body is empty\n"
        );
    }

    boost::system::error_code ec;
    auto request_value = boost::json::parse(request.body(), ec);
    if(ec || !request_value.is_object()){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "invalid json\n"
        );
    }

    const auto& request_object = request_value.as_object();
    const auto user_login_id_exp = http_util::get_non_empty_string_field(
        request_object,
        "user_login_id"
    );

    const auto raw_password_exp = http_util::get_non_empty_string_field(
        request_object,
        "raw_password"
    );

    if(!user_login_id_exp || !raw_password_exp){
        return http_util::create_text_response(
            request,
            boost::beast::http::status::bad_request,
            "required fields: user_login_id, raw_password\n"
        );
    }

    const auto sign_up_exp = login_service::sign_up(
        db_connection_,
        *user_login_id_exp,
        *raw_password_exp
    );
    
    if(!sign_up_exp){
        const auto code = sign_up_exp.error();
        const bool is_invalid_argument_error =
            code.type_ == error_type::errno_type &&
            static_cast<errno_error>(code.code_) == errno_error::invalid_argument;
        const bool is_constraint_error =
            code.type_ == error_type::psql_type &&
            (
                static_cast<psql_error>(code.code_) == psql_error::unique_violation ||
                static_cast<psql_error>(code.code_) == psql_error::check_violation ||
                static_cast<psql_error>(code.code_) == psql_error::not_null_violation
            );
        const auto status =
            (is_invalid_argument_error || is_constraint_error)
                ? boost::beast::http::status::bad_request
                : boost::beast::http::status::internal_server_error;

        return http_util::create_text_response(
            request,
            status,
            "failed to sign up: " + to_string(code) + "\n"
        );
    }

    boost::json::object response_object;
    response_object["user_id"] = sign_up_exp->user_id;
    response_object["is_admin"] = sign_up_exp->is_admin;
    response_object["token"] = sign_up_exp->token;

    auto response = http_util::create_text_response(
        request,
        boost::beast::http::status::created,
        boost::json::serialize(response_object) + "\n"
    );
    response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    return response;
}
