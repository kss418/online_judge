#pragma once

#include "common/query_param.hpp"
#include "dto/problem_dto.hpp"
#include "error/http_error.hpp"

#include <boost/json/fwd.hpp>

#include <expected>
#include <vector>

namespace problem_request_parser{
    std::expected<problem_dto::create_request, http_error> parse_create_request(
        const boost::json::object& json
    );

    std::expected<problem_dto::update_request, http_error> parse_update_request(
        const boost::json::object& json
    );

    std::expected<problem_dto::list_filter, http_error> parse_list_filter(
        const std::vector<query_param>& query_params
    );

    std::expected<void, http_error> validate_list_filter_for_viewer(
        const problem_dto::list_filter& filter_value,
        bool has_viewer_user
    );

    std::expected<problem_dto::testcase, http_error> parse_testcase(
        const boost::json::object& json
    );

    std::expected<problem_dto::testcase_move_request, http_error>
    parse_testcase_move_request(const boost::json::object& json);
}
