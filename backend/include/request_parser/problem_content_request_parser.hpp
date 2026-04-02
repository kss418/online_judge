#pragma once

#include "dto/problem_content_dto.hpp"
#include "error/http_error.hpp"

#include <boost/json/fwd.hpp>

#include <expected>

namespace problem_content_request_parser{
    std::expected<problem_content_dto::limits, http_error> parse_limits(
        const boost::json::object& json
    );

    std::expected<problem_content_dto::statement, http_error> parse_statement(
        const boost::json::object& json
    );

    std::expected<problem_content_dto::sample, http_error> parse_sample(
        const boost::json::object& json
    );
}
