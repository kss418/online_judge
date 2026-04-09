#pragma once

#include "error/judge_error.hpp"
#include "judge_core/infrastructure/execution_plan.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"
#include "judge_core/types/execution_policy.hpp"
#include "judge_core/types/execution_report.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <vector>

namespace testcase_runner{
    std::expected<execution_report::batch, judge_error> run_testcases(
        const program_launch::execution_plan& execution_plan_value,
        const testcase_snapshot& testcase_snapshot_value,
        const execution_policy& execution_policy_value
    );
}
