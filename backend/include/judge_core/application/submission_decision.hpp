#pragma once

#include "dto/submission_dto.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/judge_result.hpp"

struct submission_decision{
    judge_result judge_result_value = judge_result::wrong_answer;
    execution_report::batch execution_report_value;

    submission_dto::finalize_request to_finalize_request(
        const submission_dto::leased_submission& leased_submission_value
    ) const;
};
