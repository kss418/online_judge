#include "dto/submission_response_dto.hpp"

submission_response_dto::queued_response submission_response_dto::make_queued_response(
    std::int64_t submission_id,
    submission_status submission_status_value,
    std::int32_t problem_version
){
    queued_response queued_response_value;
    queued_response_value.submission_id = submission_id;
    queued_response_value.status = to_string(submission_status_value);
    queued_response_value.problem_version = problem_version;
    return queued_response_value;
}
