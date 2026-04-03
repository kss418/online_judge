#pragma once

#include "error/judge_error.hpp"
#include "judge_core/execution_report.hpp"
#include "judge_core/testcase_snapshot.hpp"
#include "pl_runner/pl_runner_util.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <vector>

namespace testcase_runner{
    std::expected<execution_report::testcase_execution, judge_error> run_one_testcase(
        const pl_runner_util::prepared_source& prepared_source_value,
        const std::filesystem::path& input_path,
        const problem_content_dto::limits& problem_limits_value
    );

    std::expected<execution_report::batch, judge_error> run_all_testcases(
        const std::filesystem::path& source_file_path,
        const testcase_snapshot& testcase_snapshot_value
    );
}
