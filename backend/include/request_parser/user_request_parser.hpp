#pragma once

#include "common/query_param.hpp"
#include "dto/user_dto.hpp"
#include "error/http_error.hpp"

#include <boost/json/fwd.hpp>

#include <expected>
#include <vector>

namespace user_request_parser{
    std::expected<user_dto::submission_ban_request, http_error>
    parse_submission_ban_request(const boost::json::object& json);

    std::expected<user_dto::list_filter, http_error> parse_list_filter(
        const std::vector<query_param>& query_params
    );
}
