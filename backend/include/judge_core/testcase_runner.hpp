#pragma once

#include "error/judge_error.hpp"
#include "judge_core/execution_report.hpp"
#include "judge_core/testcase_snapshot.hpp"
#include "pl_runner/program_build_types.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <vector>

namespace testcase_runner{
    std::expected<execution_report::batch, judge_error> run_all_testcases(
        const program_build::execution_plan& execution_plan_value,
        const testcase_snapshot& testcase_snapshot_value
    );
}
