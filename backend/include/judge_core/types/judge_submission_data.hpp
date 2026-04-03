#pragma once

#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/judge_result.hpp"

namespace judge_submission_data{
    struct process_submission_data{
        judge_result judge_result_value = judge_result::wrong_answer;
        execution_report::batch execution_report_value;
    };

    process_submission_data make_process_submission_data(
        judge_result judge_result_value,
        execution_report::batch&& execution_report_value
    );
}
