#pragma once

#include "common/error_code.hpp"
#include "pl_runner/pl_runner_utility.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace java_runner{
    using path = std::filesystem::path;

    struct compile_result{
        std::vector<std::string> run_command_args_;
        int exit_code_ = 0;
        std::string stderr_text_;

        bool is_success() const noexcept{
            return exit_code_ == 0;
        }
    };

    std::expected<compile_result, error_code> compile(
        const path& source_file_path,
        const path& java_runtime_path
    );

    std::expected<pl_runner_utility::prepared_source, error_code> prepare(
        const path& source_file_path,
        const path& java_runtime_path
    );
}
