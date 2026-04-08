#include "dto/submission_dto.hpp"

#include "common/language_util.hpp"

bool submission_dto::is_valid(const source& source_value){
    return
        !source_value.language.empty() &&
        !source_value.source_code.empty() &&
        language_util::find_supported_language(source_value.language).has_value();
}

bool submission_dto::is_valid(const create_request& create_request_value){
    return
        create_request_value.user_id > 0 &&
        create_request_value.problem_id > 0 &&
        is_valid(create_request_value.source_value);
}

bool submission_dto::is_valid(const lease_request& lease_request_value){
    return lease_request_value.lease_duration > std::chrono::seconds::zero();
}

bool submission_dto::is_valid(const status_update& status_update_value){
    return
        status_update_value.submission_id > 0 &&
        status_update_value.attempt_no > 0 &&
        !status_update_value.lease_token.empty();
}

bool submission_dto::is_valid(const finalize_request& finalize_request_value){
    return
        finalize_request_value.submission_id > 0 &&
        finalize_request_value.attempt_no > 0 &&
        !finalize_request_value.lease_token.empty();
}

submission_dto::created submission_dto::make_created(
    std::int64_t submission_id,
    submission_status submission_status_value
){
    created created_value;
    created_value.submission_id = submission_id;
    created_value.status = to_string(submission_status_value);
    return created_value;
}

std::optional<submission_status> submission_dto::make_submission_status(
    std::string_view submission_status_string
){
    return parse_submission_status(submission_status_string);
}

submission_dto::status_update submission_dto::make_status_update(
    const leased_submission& leased_submission_value,
    submission_status to_status,
    std::optional<std::string> reason_opt
){
    status_update status_update_value;
    status_update_value.submission_id = leased_submission_value.submission_id;
    status_update_value.attempt_no = leased_submission_value.attempt_no;
    status_update_value.lease_token = leased_submission_value.lease_token;
    status_update_value.to_status = to_status;
    status_update_value.reason_opt = std::move(reason_opt);
    return status_update_value;
}

submission_dto::finalize_request submission_dto::make_finalize_request(
    const leased_submission& leased_submission_value,
    submission_status to_status,
    std::optional<std::int16_t> score_opt,
    std::optional<std::string> compile_output_opt,
    std::optional<std::string> judge_output_opt,
    std::optional<std::int64_t> elapsed_ms_opt,
    std::optional<std::int64_t> max_rss_kb_opt,
    std::optional<std::string> reason_opt
){
    finalize_request finalize_request_value;
    finalize_request_value.submission_id = leased_submission_value.submission_id;
    finalize_request_value.attempt_no = leased_submission_value.attempt_no;
    finalize_request_value.lease_token = leased_submission_value.lease_token;
    finalize_request_value.to_status = to_status;
    finalize_request_value.score_opt = std::move(score_opt);
    finalize_request_value.compile_output_opt = std::move(compile_output_opt);
    finalize_request_value.judge_output_opt = std::move(judge_output_opt);
    finalize_request_value.elapsed_ms_opt = std::move(elapsed_ms_opt);
    finalize_request_value.max_rss_kb_opt = std::move(max_rss_kb_opt);
    finalize_request_value.reason_opt = std::move(reason_opt);
    return finalize_request_value;
}

submission_dto::finalize_result submission_dto::make_finalize_result(
    std::int64_t problem_id,
    bool should_increase_accepted_count
){
    finalize_result finalize_result_value;
    finalize_result_value.problem_id = problem_id;
    finalize_result_value.should_increase_accepted_count = should_increase_accepted_count;
    return finalize_result_value;
}
