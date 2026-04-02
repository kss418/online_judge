#pragma once

#include "common/query_param.hpp"
#include "dto/submission_dto.hpp"
#include "error/http_error.hpp"

#include <boost/json/fwd.hpp>

#include <expected>
#include <vector>

namespace submission_request_parser{
    std::expected<submission_dto::source, http_error> parse_source(
        const boost::json::object& json
    );

    std::expected<submission_dto::status_batch_request, http_error>
    parse_status_batch_request(const boost::json::object& json);

    std::expected<submission_dto::list_filter, http_error> parse_list_filter(
        const std::vector<query_param>& query_params
    );

    std::expected<submission_dto::create_request, http_error> parse_create_request(
        const boost::json::object& json,
        std::int64_t user_id,
        std::int64_t problem_id
    );
}
