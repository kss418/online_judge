#pragma once

#include "common/error_code.hpp"
#include "judge_server/sandbox_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

class code_runner{
public:
    static std::expected<code_runner, error_code> create();

    std::expected<sandbox_runner::run_result, error_code> run_one_testcase(
        const std::filesystem::path& source_file_path,
        const std::filesystem::path& input_path
    );
    std::expected<std::vector<sandbox_runner::run_result>, error_code> run_all_testcases(
        const std::filesystem::path& source_file_path,
        std::int64_t problem_id
    );

private:
    code_runner(
        std::string cpp_compiler_path,
        std::string python_path,
        std::string java_runtime_path
    );

    std::expected<std::filesystem::path, error_code> make_input_path(
        std::int64_t problem_id,
        std::int32_t order
    );
    
    std::expected<std::filesystem::path, error_code> make_output_path(
        std::int64_t problem_id,
        std::int32_t order
    );

    static constexpr std::chrono::milliseconds source_run_time_limit_{2000};
    static constexpr std::int64_t source_run_memory_limit_mb_{256};

    std::string cpp_compiler_path_;
    std::string python_path_;
    std::string java_runtime_path_;
};
