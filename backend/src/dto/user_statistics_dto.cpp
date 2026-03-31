#include "dto/user_statistics_dto.hpp"

#include "common/row_util.hpp"

#include <pqxx/pqxx>

user_statistics_dto::submission_statistics user_statistics_dto::make_submission_statistics_from_row(
    const pqxx::row& user_submission_statistics_row
){
    user_statistics_dto::submission_statistics statistics_value;
    statistics_value.user_id = row_util::get_required<std::int64_t>(user_submission_statistics_row, 0);
    statistics_value.submission_count = row_util::get_required<std::int64_t>(user_submission_statistics_row, 1);
    statistics_value.queued_submission_count = row_util::get_required<std::int64_t>(user_submission_statistics_row, 2);
    statistics_value.judging_submission_count = row_util::get_required<std::int64_t>(user_submission_statistics_row, 3);
    statistics_value.accepted_submission_count = row_util::get_required<std::int64_t>(user_submission_statistics_row, 4);
    statistics_value.wrong_answer_submission_count = row_util::get_required<std::int64_t>(user_submission_statistics_row, 5);
    statistics_value.time_limit_exceeded_submission_count =
        row_util::get_required<std::int64_t>(user_submission_statistics_row, 6);
    statistics_value.memory_limit_exceeded_submission_count =
        row_util::get_required<std::int64_t>(user_submission_statistics_row, 7);
    statistics_value.runtime_error_submission_count =
        row_util::get_required<std::int64_t>(user_submission_statistics_row, 8);
    statistics_value.compile_error_submission_count =
        row_util::get_required<std::int64_t>(user_submission_statistics_row, 9);
    statistics_value.output_exceeded_submission_count =
        row_util::get_required<std::int64_t>(user_submission_statistics_row, 10);
    statistics_value.last_submission_at_opt =
        row_util::get_optional<std::string>(user_submission_statistics_row, 11);
    statistics_value.last_accepted_at_opt =
        row_util::get_optional<std::string>(user_submission_statistics_row, 12);
    statistics_value.updated_at = row_util::get_required<std::string>(user_submission_statistics_row, 13);
    return statistics_value;
}
