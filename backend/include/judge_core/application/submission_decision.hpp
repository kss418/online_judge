#pragma once

#include "dto/submission_dto.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/judge_result.hpp"

struct submission_verdict_summary{
    judge_result overall_verdict = judge_result::wrong_answer;
    std::optional<std::int32_t> first_failed_case_index_opt = std::nullopt;
    std::optional<std::string> public_message_opt = std::nullopt;
    std::optional<std::string> internal_message_opt = std::nullopt;
    std::optional<std::int16_t> score_opt = std::nullopt;
};

struct submission_decision{
    submission_verdict_summary verdict_summary;
    execution_report::batch execution_report_value;

    submission_dto::finalize_request to_finalize_request(
        const submission_dto::leased_submission& leased_submission_value
    ) const;
};
