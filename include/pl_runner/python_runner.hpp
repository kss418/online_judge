#pragma once

#include "common/error_code.hpp"
#include "judge_server/sandbox_runner.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace python_runner{
    using path = std::filesystem::path;

    struct compile_result{
        path source_file_path_;
        std::vector<std::string> run_command_args_;
        int exit_code_ = 0;
        std::string stderr_text_;

        bool is_success() const noexcept{
            return exit_code_ == 0;
        }
    };

    std::expected<compile_result, error_code> compile(
        const path& source_file_path,
        const path& python_path
    );

    std::expected<sandbox_runner::run_result, error_code> run(
        const compile_result& compile_result_value,
        const path& input_path,
        std::chrono::milliseconds time_limit,
        std::int64_t memory_limit_mb
    );
}
