#include "row_mapper/problem_row_mapper.hpp"

#include "common/row_util.hpp"

#include <pqxx/pqxx>

problem_dto::summary problem_row_mapper::map_summary_row(
    const pqxx::row& problem_summary_row
){
    problem_dto::summary summary_value;
    summary_value.problem_id = row_util::get_required<std::int64_t>(problem_summary_row, 0);
    summary_value.title = row_util::get_required<std::string>(problem_summary_row, 1);
    summary_value.version = row_util::get_required<std::int32_t>(problem_summary_row, 2);
    summary_value.time_limit_ms = row_util::get_required<std::int32_t>(problem_summary_row, 3);
    summary_value.memory_limit_mb = row_util::get_required<std::int32_t>(problem_summary_row, 4);
    summary_value.submission_count = row_util::get_required<std::int64_t>(problem_summary_row, 5);
    summary_value.accepted_count = row_util::get_required<std::int64_t>(problem_summary_row, 6);
    summary_value.user_problem_state_opt = row_util::get_optional<std::string>(problem_summary_row, 7);
    return summary_value;
}

std::vector<problem_dto::summary> problem_row_mapper::map_summary_result(
    const pqxx::result& problem_summary_result
){
    std::vector<problem_dto::summary> summary_values;
    summary_values.reserve(problem_summary_result.size());
    for(const auto& problem_summary_row : problem_summary_result){
        summary_values.push_back(map_summary_row(problem_summary_row));
    }
    return summary_values;
}
