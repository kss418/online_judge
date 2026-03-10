#pragma once

#include "common/error_code.hpp"
#include "judge_server/sandbox_runner.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <vector>

namespace testcase_runner{
    std::expected<std::vector<sandbox_runner::run_result>, error_code> run_all_testcases(
        const std::filesystem::path& source_file_path,
        std::int64_t problem_id
    );
}
