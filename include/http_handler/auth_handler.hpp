#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <string>
#include <string_view>

class auth_handler{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    auth_handler(const auth_handler&) = delete;
    auth_handler& operator=(const auth_handler&) = delete;
    auth_handler(auth_handler&&) noexcept = delete;
    auth_handler& operator=(auth_handler&&) noexcept = delete;

    explicit auth_handler(db_connection& db_connection);
    static bool is_auth_path(std::string_view path);
    response_type handle(const request_type& request, std::string_view path);
    response_type handle_sign_up_post(const request_type& request);
    response_type handle_login_post(const request_type& request);
    response_type handle_token_renew_post(const request_type& request);
    response_type handle_logout_post(const request_type& request);

private:
    static constexpr std::string_view path_prefix_ = "/api/auth";

    db_connection& db_connection_;
};
