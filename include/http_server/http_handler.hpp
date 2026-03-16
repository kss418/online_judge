#pragma once

#include "common/error_code.hpp"
#include "db/submission_service.hpp"

#include <array>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/string_body.hpp>

#include <expected>
#include <optional>
#include <string>
#include <string_view>

class http_handler{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    static std::expected<http_handler, error_code> create(
        submission_service submission_service
    );

    response_type handle(const request_type& request);

private:
    using route_handler = response_type (http_handler::*)(const request_type&);

    struct route_definition{
        boost::beast::http::verb method;
        std::string_view path;
        route_handler handler;
    };

    explicit http_handler(submission_service submission_service);

    static response_type create_text_response(
        const request_type& request, boost::beast::http::status status, std::string body
    );
    static std::optional<route_handler> find_route_handler(
        boost::beast::http::verb method,
        std::string_view path
    );
    static bool has_route_path(std::string_view path);

    response_type handle_health_get(const request_type& request);
    response_type handle_submission(const request_type& request);

    static constexpr std::string_view health_path_ = "/api/health";
    static constexpr std::string_view submissions_path_ = "/api/submissions";
    static constexpr std::array<route_definition, 2> routes_{{
        route_definition{
            boost::beast::http::verb::get,
            health_path_,
            &http_handler::handle_health_get
        },
        route_definition{
            boost::beast::http::verb::post,
            submissions_path_,
            &http_handler::handle_submission
        }
    }};

    submission_service submission_service_;
};
