#include "row_mapper/submission_row_mapper.hpp"

#include "common/row_util.hpp"

#include <pqxx/pqxx>

submission_dto::history submission_row_mapper::map_history_row(
    const pqxx::row& submission_history_row
){
    submission_dto::history history_value;
    history_value.history_id = row_util::get_required<std::int64_t>(submission_history_row, 0);
    history_value.from_status_opt = row_util::get_optional<std::string>(submission_history_row, 1);
    history_value.to_status = row_util::get_required<std::string>(submission_history_row, 2);
    history_value.reason_opt = row_util::get_optional<std::string>(submission_history_row, 3);
    history_value.created_at = row_util::get_required<std::string>(submission_history_row, 4);
    return history_value;
}

submission_dto::history_list submission_row_mapper::map_history_result(
    const pqxx::result& submission_history_result
){
    submission_dto::history_list history_values;
    history_values.reserve(submission_history_result.size());
    for(const auto& submission_history_row : submission_history_result){
        history_values.push_back(map_history_row(submission_history_row));
    }
    return history_values;
}

submission_dto::source_detail submission_row_mapper::map_source_detail_row(
    const pqxx::row& submission_source_row
){
    submission_dto::source_detail source_detail_value;
    source_detail_value.submission_id = row_util::get_required<std::int64_t>(submission_source_row, 0);
    source_detail_value.user_id = row_util::get_required<std::int64_t>(submission_source_row, 1);
    source_detail_value.problem_id = row_util::get_required<std::int64_t>(submission_source_row, 2);
    source_detail_value.language = row_util::get_required<std::string>(submission_source_row, 3);
    source_detail_value.source_code = row_util::get_required<std::string>(submission_source_row, 4);
    source_detail_value.compile_output_opt = row_util::get_optional<std::string>(submission_source_row, 5);
    source_detail_value.judge_output_opt = row_util::get_optional<std::string>(submission_source_row, 6);
    return source_detail_value;
}

submission_dto::detail submission_row_mapper::map_detail_row(
    const pqxx::row& submission_detail_row
){
    submission_dto::detail detail_value;
    detail_value.submission_id = row_util::get_required<std::int64_t>(submission_detail_row, 0);
    detail_value.user_id = row_util::get_required<std::int64_t>(submission_detail_row, 1);
    detail_value.problem_id = row_util::get_required<std::int64_t>(submission_detail_row, 2);
    detail_value.language = row_util::get_required<std::string>(submission_detail_row, 3);
    detail_value.status = row_util::get_required<std::string>(submission_detail_row, 4);
    detail_value.score_opt = row_util::get_optional<std::int16_t>(submission_detail_row, 5);
    detail_value.compile_output_opt = row_util::get_optional<std::string>(submission_detail_row, 6);
    detail_value.judge_output_opt = row_util::get_optional<std::string>(submission_detail_row, 7);
    detail_value.elapsed_ms_opt = row_util::get_optional<std::int64_t>(submission_detail_row, 8);
    detail_value.max_rss_kb_opt = row_util::get_optional<std::int64_t>(submission_detail_row, 9);
    detail_value.created_at = row_util::get_required<std::string>(submission_detail_row, 10);
    detail_value.updated_at = row_util::get_required<std::string>(submission_detail_row, 11);
    return detail_value;
}

submission_dto::status_snapshot submission_row_mapper::map_status_snapshot_row(
    const pqxx::row& submission_status_row
){
    submission_dto::status_snapshot snapshot_value;
    snapshot_value.submission_id = row_util::get_required<std::int64_t>(submission_status_row, 0);
    snapshot_value.status = row_util::get_required<std::string>(submission_status_row, 1);
    snapshot_value.score_opt = row_util::get_optional<std::int16_t>(submission_status_row, 2);
    snapshot_value.elapsed_ms_opt = row_util::get_optional<std::int64_t>(submission_status_row, 3);
    snapshot_value.max_rss_kb_opt = row_util::get_optional<std::int64_t>(submission_status_row, 4);
    return snapshot_value;
}

std::vector<submission_dto::status_snapshot> submission_row_mapper::map_status_snapshot_result(
    const pqxx::result& submission_status_result
){
    std::vector<submission_dto::status_snapshot> snapshot_values;
    snapshot_values.reserve(submission_status_result.size());
    for(const auto& submission_status_row : submission_status_result){
        snapshot_values.push_back(map_status_snapshot_row(submission_status_row));
    }
    return snapshot_values;
}

std::expected<submission_status, repository_error> submission_row_mapper::map_submission_status_row(
    const pqxx::row& submission_row,
    std::size_t status_column_index
){
    if(status_column_index >= submission_row.size()){
        return std::unexpected(repository_error::internal);
    }

    const auto submission_status_string_opt =
        row_util::get_optional<std::string>(submission_row, status_column_index);
    if(!submission_status_string_opt){
        return std::unexpected(repository_error::internal);
    }

    const auto submission_status_opt =
        submission_dto::make_submission_status(*submission_status_string_opt);
    if(!submission_status_opt){
        return std::unexpected(repository_error::internal);
    }

    return *submission_status_opt;
}

submission_dto::summary submission_row_mapper::map_summary_row(
    const pqxx::row& submission_summary_row
){
    submission_dto::summary summary_value;
    summary_value.submission_id = row_util::get_required<std::int64_t>(submission_summary_row, 0);
    summary_value.user_id = row_util::get_required<std::int64_t>(submission_summary_row, 1);
    summary_value.user_login_id = row_util::get_required<std::string>(submission_summary_row, 2);
    summary_value.problem_id = row_util::get_required<std::int64_t>(submission_summary_row, 3);
    summary_value.problem_title = row_util::get_required<std::string>(submission_summary_row, 4);
    summary_value.language = row_util::get_required<std::string>(submission_summary_row, 5);
    summary_value.status = row_util::get_required<std::string>(submission_summary_row, 6);
    summary_value.score_opt = row_util::get_optional<std::int16_t>(submission_summary_row, 7);
    summary_value.elapsed_ms_opt = row_util::get_optional<std::int64_t>(submission_summary_row, 8);
    summary_value.max_rss_kb_opt = row_util::get_optional<std::int64_t>(submission_summary_row, 9);
    summary_value.user_problem_state_opt = row_util::get_optional<std::string>(submission_summary_row, 10);
    summary_value.created_at = row_util::get_required<std::string>(submission_summary_row, 11);
    summary_value.updated_at = row_util::get_required<std::string>(submission_summary_row, 12);
    return summary_value;
}

std::vector<submission_dto::summary> submission_row_mapper::map_summary_result(
    const pqxx::result& submission_summary_result
){
    std::vector<submission_dto::summary> summary_values;
    summary_values.reserve(submission_summary_result.size());
    for(const auto& submission_summary_row : submission_summary_result){
        summary_values.push_back(map_summary_row(submission_summary_row));
    }
    return summary_values;
}

submission_dto::leased_submission submission_row_mapper::map_leased_submission_row(
    const pqxx::row& submission_queue_row
){
    submission_dto::leased_submission leased_submission_value;
    leased_submission_value.submission_id = row_util::get_required<std::int64_t>(submission_queue_row, 0);
    leased_submission_value.problem_id = row_util::get_required<std::int64_t>(submission_queue_row, 1);
    leased_submission_value.problem_version = row_util::get_required<std::int32_t>(submission_queue_row, 2);
    leased_submission_value.queue_wait_ms = row_util::get_required<std::int64_t>(submission_queue_row, 3);
    leased_submission_value.attempt_no = row_util::get_required<std::int32_t>(submission_queue_row, 4);
    leased_submission_value.lease_token = row_util::get_required<std::string>(submission_queue_row, 5);
    leased_submission_value.leased_until = row_util::get_required<std::string>(submission_queue_row, 6);
    leased_submission_value.language = row_util::get_required<std::string>(submission_queue_row, 7);
    leased_submission_value.source_code = row_util::get_required<std::string>(submission_queue_row, 8);
    return leased_submission_value;
}
