#pragma once

#include "common/query_param.hpp"

#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <optional>
#include <string_view>
#include <vector>

namespace request_parser{
    using request_type = boost::beast::http::request<boost::beast::http::string_body>;

    std::string_view get_target_path(std::string_view target);
    std::optional<std::string_view> get_target_query(std::string_view target);
    std::optional<std::vector<query_param>> parse_query_params(
        std::string_view query
    );
    std::optional<boost::json::object> parse_json_object(
        const request_type& request
    );
    std::optional<std::vector<std::string_view>> parse_path(
        std::string_view prefix,
        std::string_view path
    );
}
