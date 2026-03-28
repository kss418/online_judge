#include "dto/user_dto.hpp"

#include "http_core/http_util.hpp"
#include "http_core/query_param_util.hpp"

#include <pqxx/pqxx>

std::expected<user_dto::list_filter, dto_validation_error>
user_dto::make_list_filter_from_query_params(
    const std::vector<http_util::query_param>& query_params
){
    list_filter filter_value;
    for(const auto& query_param : query_params){
        if(query_param.key == "q"){
            const auto parse_query_exp = query_param_util::parse_unique_query_param(
                filter_value.query_opt,
                query_param.key,
                query_param.value,
                [](std::string_view raw_value) -> std::optional<std::string> {
                    if(raw_value.empty()){
                        return std::nullopt;
                    }

                    return std::string{raw_value};
                }
            );
            if(!parse_query_exp){
                return std::unexpected(parse_query_exp.error());
            }
            continue;
        }

        return std::unexpected(
            query_param_util::make_unsupported_query_parameter_error(query_param.key)
        );
    }

    return filter_value;
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
