#include "row_mapper/user_row_mapper.hpp"

#include "common/row_util.hpp"

#include <pqxx/pqxx>

user_dto::summary user_row_mapper::map_summary_row(const pqxx::row& user_summary_row){
    user_dto::summary summary_value;
    summary_value.user_id = row_util::get_required<std::int64_t>(user_summary_row, 0);
    summary_value.user_login_id = row_util::get_required<std::string>(user_summary_row, 1);
    summary_value.created_at = row_util::get_required<std::string>(user_summary_row, 2);
    return summary_value;
}

user_dto::list_item user_row_mapper::map_list_item_row(const pqxx::row& user_list_row){
    user_dto::list_item item_value;
    item_value.user_id = row_util::get_required<std::int64_t>(user_list_row, 0);
    item_value.user_login_id = row_util::get_required<std::string>(user_list_row, 1);
    item_value.solved_problem_count = row_util::get_required<std::int64_t>(user_list_row, 2);
    item_value.accepted_submission_count = row_util::get_required<std::int64_t>(user_list_row, 3);
    item_value.submission_count = row_util::get_required<std::int64_t>(user_list_row, 4);
    item_value.created_at = row_util::get_required<std::string>(user_list_row, 5);
    return item_value;
}

user_dto::list user_row_mapper::map_list_result(const pqxx::result& user_list_result){
    user_dto::list item_values;
    item_values.reserve(user_list_result.size());
    for(const auto& user_list_row : user_list_result){
        item_values.push_back(map_list_item_row(user_list_row));
    }
    return item_values;
}
