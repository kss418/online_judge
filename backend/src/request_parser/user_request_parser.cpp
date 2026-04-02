#include "request_parser/user_request_parser.hpp"

#include "common/json_field_util.hpp"
#include "common/query_param_util.hpp"
#include "error/request_error.hpp"

#include <array>
#include <optional>
#include <string>

namespace{
    http_error make_duplicate_query_parameter_error(std::string_view key){
        return request_error::make_duplicate_query_parameter_error(key);
    }

    http_error make_invalid_query_parameter_error(std::string_view key){
        return request_error::make_invalid_query_parameter_error(key);
    }

    http_error make_unsupported_query_parameter_error(std::string_view key){
        return request_error::make_unsupported_query_parameter_error(key);
    }

    const std::array<
        query_param_util::query_param_binding<user_dto::list_filter>,
        1
    > user_list_filter_bindings{{
        {
            "q",
            [](user_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, http_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.query_opt,
                    key,
                    raw_value,
                    [](std::string_view value) -> std::optional<std::string> {
                        if(value.empty()){
                            return std::nullopt;
                        }

                        return std::string{value};
                    },
                    make_duplicate_query_parameter_error,
                    make_invalid_query_parameter_error
                );
            }
        }
    }};
}

std::expected<user_dto::submission_ban_request, http_error>
user_request_parser::parse_submission_ban_request(const boost::json::object& json){
    const auto duration_minutes_opt = json_field_util::get_positive_int32_field(
        json,
        "duration_minutes"
    );
    if(!duration_minutes_opt){
        return std::unexpected(request_error::make_invalid_field_error(
            "duration_minutes",
            "duration_minutes must be a positive integer"
        ));
    }

    user_dto::submission_ban_request request_value;
    request_value.duration_minutes = *duration_minutes_opt;
    return request_value;
}

std::expected<user_dto::list_filter, http_error>
user_request_parser::parse_list_filter(const std::vector<query_param>& query_params){
    return query_param_util::make_filter_from_query_params(
        query_params,
        user_list_filter_bindings,
        make_unsupported_query_parameter_error
    );
}
