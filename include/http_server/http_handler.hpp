#pragma once

#include "common/error_code.hpp"
#include "db/submission_service.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <string>

class http_handler{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    static std::expected<http_handler, error_code> create(
        submission_service submission_service
    );

    response_type handle(const request_type& request);

private:
    explicit http_handler(submission_service submission_service);

    static response_type create_text_response(
        const request_type& request, boost::beast::http::status status, std::string body
    );

    response_type handle_health_get(const request_type& request);
    response_type handle_submission(const request_type& request);

    submission_service submission_service_;
};
