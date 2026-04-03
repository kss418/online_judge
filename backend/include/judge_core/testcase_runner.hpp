#pragma once

#include "error/judge_error.hpp"
#include "judge_core/sandbox_runner.hpp"
#include "judge_core/testcase_snapshot.hpp"
#include "pl_runner/pl_runner_util.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <vector>

namespace testcase_runner{
    struct run_batch{
        std::vector<sandbox_runner::run_result> run_results;
        bool compile_failed = false;
        std::int32_t testcase_count = 0;
        std::int64_t prepare_elapsed_ms = 0;
        std::int64_t testcase_execution_elapsed_ms = 0;
    };

    std::expected<sandbox_runner::run_result, judge_error> run_one_testcase(
        const pl_runner_util::prepared_source& prepared_source_value,
        const std::filesystem::path& input_path,
        const problem_content_dto::limits& problem_limits_value
    );

    std::expected<run_batch, judge_error> run_all_testcases(
        const std::filesystem::path& source_file_path,
        const testcase_snapshot& testcase_snapshot_value
    );
}
