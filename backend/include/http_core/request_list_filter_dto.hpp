#pragma once

#include "dto/submission_request_dto.hpp"
#include "dto/problem_dto.hpp"
#include "http_core/request_dto.hpp"

#include <expected>

namespace request_dto{
    std::expected<submission_request_dto::list_filter, response_type>
    parse_submission_list_filter_or_400(
        const request_type& request
    );

    std::expected<problem_dto::list_filter, response_type>
    parse_problem_list_filter_or_400(
        const request_type& request
    );
}
