#include "dto/user_dto.hpp"

#include "http_core/http_util.hpp"
#include "http_core/query_param_util.hpp"

#include <pqxx/pqxx>

namespace{
    const std::array<
        query_param_util::query_param_binding<user_dto::list_filter>,
        1
    > user_list_filter_bindings{{
        {
            "q",
            [](user_dto::list_filter& filter_value,
               std::string_view key,
               std::string_view raw_value) -> std::expected<void, dto_validation_error> {
                return query_param_util::parse_unique_query_param(
                    filter_value.query_opt,
                    key,
                    raw_value,
                    [](std::string_view value) -> std::optional<std::string> {
                        if(value.empty()){
                            return std::nullopt;
                        }

                        return std::string{value};
                    }
                );
            }
        }
    }};
}

std::expected<user_dto::submission_ban_request, dto_validation_error>
user_dto::make_submission_ban_request_from_json(const boost::json::object& json){
    const auto duration_minutes_opt = http_util::get_positive_int32_field(
        json,
        "duration_minutes"
    );
    if(!duration_minutes_opt){
        return std::unexpected(dto_validation_error{
            .code = "invalid_field",
            .message = "duration_minutes must be a positive integer",
            .field_opt = "duration_minutes"
        });
    }

    submission_ban_request request_value;
    request_value.duration_minutes = *duration_minutes_opt;
    return request_value;
}

std::expected<user_dto::list_filter, dto_validation_error>
user_dto::make_list_filter_from_query_params(
    const std::vector<http_util::query_param>& query_params
){
    return query_param_util::make_filter_from_query_params(
        query_params,
        user_list_filter_bindings
    );
}

user_dto::summary user_dto::make_summary_from_row(const pqxx::row& user_summary_row){
    user_dto::summary summary_value;
    summary_value.user_id = user_summary_row[0].as<std::int64_t>();
    summary_value.user_login_id = user_summary_row[1].as<std::string>();
    summary_value.created_at = user_summary_row[2].as<std::string>();
    return summary_value;
}

user_dto::list_item user_dto::make_list_item_from_row(const pqxx::row& user_list_row){
    user_dto::list_item item_value;
    item_value.user_id = user_list_row[0].as<std::int64_t>();
    item_value.user_login_id = user_list_row[1].as<std::string>();
    item_value.solved_problem_count = user_list_row[2].as<std::int64_t>();
    item_value.accepted_submission_count = user_list_row[3].as<std::int64_t>();
    item_value.submission_count = user_list_row[4].as<std::int64_t>();
    item_value.created_at = user_list_row[5].as<std::string>();
    return item_value;
}

user_dto::list user_dto::make_list_from_result(const pqxx::result& user_list_result){
    list item_values;
    item_values.reserve(user_list_result.size());
    for(const auto& user_list_row : user_list_result){
        item_values.push_back(make_list_item_from_row(user_list_row));
    }
    return item_values;
}
