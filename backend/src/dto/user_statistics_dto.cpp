#include "dto/user_statistics_dto.hpp"

#include <pqxx/pqxx>

user_statistics_dto::submission_statistics user_statistics_dto::make_submission_statistics_from_row(
    const pqxx::row& user_submission_statistics_row
){
    user_statistics_dto::submission_statistics statistics_value;
    statistics_value.user_id = user_submission_statistics_row[0].as<std::int64_t>();
    statistics_value.submission_count = user_submission_statistics_row[1].as<std::int64_t>();
    statistics_value.queued_submission_count = user_submission_statistics_row[2].as<std::int64_t>();
    statistics_value.judging_submission_count = user_submission_statistics_row[3].as<std::int64_t>();
    statistics_value.accepted_submission_count = user_submission_statistics_row[4].as<std::int64_t>();
    statistics_value.wrong_answer_submission_count = user_submission_statistics_row[5].as<std::int64_t>();
    statistics_value.time_limit_exceeded_submission_count =
        user_submission_statistics_row[6].as<std::int64_t>();
    statistics_value.memory_limit_exceeded_submission_count =
        user_submission_statistics_row[7].as<std::int64_t>();
    statistics_value.runtime_error_submission_count = user_submission_statistics_row[8].as<std::int64_t>();
    statistics_value.compile_error_submission_count = user_submission_statistics_row[9].as<std::int64_t>();
    statistics_value.output_exceeded_submission_count = user_submission_statistics_row[10].as<std::int64_t>();
    if(!user_submission_statistics_row[11].is_null()){
        statistics_value.last_submission_at_opt = user_submission_statistics_row[11].as<std::string>();
    }
    if(!user_submission_statistics_row[12].is_null()){
        statistics_value.last_accepted_at_opt = user_submission_statistics_row[12].as<std::string>();
    }
    statistics_value.updated_at = user_submission_statistics_row[13].as<std::string>();
    return statistics_value;
}
