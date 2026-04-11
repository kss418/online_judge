#include "dto/submission_dto.hpp"

bool submission_dto::is_valid(const source& source_value){
    return submission_request_dto::is_valid(source_value);
}

bool submission_dto::is_valid(const create_request& create_request_value){
    return submission_internal_dto::is_valid(create_request_value);
}

bool submission_dto::is_valid(const lease_request& lease_request_value){
    return submission_internal_dto::is_valid(lease_request_value);
}

bool submission_dto::is_valid(const status_update& status_update_value){
    return submission_internal_dto::is_valid(status_update_value);
}

bool submission_dto::is_valid(const finalize_request& finalize_request_value){
    return submission_internal_dto::is_valid(finalize_request_value);
}

submission_dto::queued_response submission_dto::make_queued_response(
    std::int64_t submission_id,
    submission_status submission_status_value,
    std::int32_t problem_version
){
    return submission_response_dto::make_queued_response(
        submission_id,
        submission_status_value,
        problem_version
    );
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
    return submission_internal_dto::make_status_update(
        leased_submission_value,
        to_status,
        std::move(reason_opt)
    );
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
    return submission_internal_dto::make_finalize_request(
        leased_submission_value,
        to_status,
        std::move(score_opt),
        std::move(compile_output_opt),
        std::move(judge_output_opt),
        std::move(elapsed_ms_opt),
        std::move(max_rss_kb_opt),
        std::move(reason_opt)
    );
}

submission_dto::finalize_result submission_dto::make_finalize_result(
    std::int64_t problem_id,
    bool should_increase_accepted_count
){
    return submission_internal_dto::make_finalize_result(
        problem_id,
        should_increase_accepted_count
    );
}
