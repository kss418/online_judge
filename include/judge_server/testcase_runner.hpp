#pragma once

#include "common/error_code.hpp"
#include "dto/problem_dto.hpp"
#include "judge_server/sandbox_runner.hpp"
#include "pl_runner/pl_runner_util.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <vector>

namespace testcase_runner{
    std::expected<problem_dto::limits, error_code> read_problem_limits(std::int64_t problem_id);

    std::expected<sandbox_runner::run_result, error_code> run_one_testcase(
        const pl_runner_util::prepared_source& prepared_source_value,
        const std::filesystem::path& input_path,
        const problem_dto::limits& problem_limits_value
    );

    std::expected<std::vector<sandbox_runner::run_result>, error_code> run_all_testcases(
        const std::filesystem::path& source_file_path,
        std::int64_t problem_id
    );
}
