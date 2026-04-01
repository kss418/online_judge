#include "http_core/request_list_filter_dto.hpp"

std::expected<submission_dto::list_filter, request_dto::response_type>
request_dto::parse_submission_list_filter_or_400(
    const request_type& request
){
    return parse_query_dto_or_400<submission_dto::list_filter>(
        request,
        submission_dto::make_list_filter_from_query_params
    );
}

std::expected<problem_dto::list_filter, request_dto::response_type>
request_dto::parse_problem_list_filter_or_400(
    const request_type& request
){
    return parse_query_dto_or_400<problem_dto::list_filter>(
        request,
        problem_dto::make_list_filter_from_query_params
    );
}
