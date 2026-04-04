#pragma once

#include "common/submission_status.hpp"
#include "error/judge_error.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/judge_expectation.hpp"
#include "judge_core/types/judge_result.hpp"

#include <expected>
#include <vector>

namespace judge_policy{
    std::expected<judge_result, judge_error> check_result(
        const judge_expectation& judge_expectation_value,
        const execution_report::batch& execution_report_value
    );

    submission_status to_submission_status(judge_result result);
}
