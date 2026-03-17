#pragma once

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <string_view>

class system_handler{
public:
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;
    using response_type = boost::beast::http::response<boost::beast::http::string_body>;

    static bool is_system_path(std::string_view path);
    response_type handle(const request_type& request, std::string_view path);
    response_type handle_health_get(const request_type& request);

private:
    static constexpr std::string_view path_prefix_ = "/api/system";
};
