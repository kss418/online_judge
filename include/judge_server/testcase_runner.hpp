#pragma once

#include "common/error_code.hpp"
#include "dto/problem_dto.hpp"
#include "judge_server/sandbox_runner.hpp"
#include "pl_runner/pl_runner_util.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <vector>

namespace tc_runner{
    std::expected<limits, error_code> read_problem_limits(std::int64_t problem_id);

    std::expected<sandbox_runner::run_result, error_code> run_one_tc(
        const pl_runner_util::prepared_source& prepared_source_value,
        const std::filesystem::path& input_path,
        const limits& problem_limits_value
    );

    std::expected<std::vector<sandbox_runner::run_result>, error_code> run_all_tcs(
        const std::filesystem::path& source_file_path,
        std::int64_t problem_id
    );
}
