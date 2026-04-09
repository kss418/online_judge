#pragma once

#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/judge_result.hpp"

#include <optional>

namespace execution_failure_classifier{
    std::optional<execution_report::execution_failure_kind> classify(
        const execution_report::testcase_execution& testcase_execution_value,
        const execution_report::applied_limits& limits_value
    );

    judge_result to_judge_result(
        execution_report::execution_failure_kind failure_kind_value
    );
}
