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

class submission_handler{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    submission_handler(const submission_handler&) = delete;
    submission_handler& operator=(const submission_handler&) = delete;
    submission_handler(submission_handler&&) noexcept = delete;
    submission_handler& operator=(submission_handler&&) noexcept = delete;

    explicit submission_handler(db_connection& db_connection);
    static bool is_submission_path(std::string_view path);
    response_type handle_create_submission_post(const request_type& request);

private:
    static constexpr std::string_view path_prefix_ = "/api/submission";

    db_connection& db_connection_;
};
