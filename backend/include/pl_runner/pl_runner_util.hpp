#pragma once

#include "common/error_code.hpp"
#include "common/temp_file.hpp"
#include "judge_core/sandbox_runner.hpp"

#include <mutex>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class pl_runner_util{
public:
    struct prepared_source{
        std::filesystem::path workspace_host_path_;
        std::vector<std::string> run_command_args_;
        std::optional<sandbox_runner::run_result> compile_failed_run_result_;
        sandbox_runner::mount_profile mount_profile_ =
            sandbox_runner::mount_profile::default_profile;

        bool is_runnable() const noexcept{
            return !compile_failed_run_result_.has_value();
        }
    };

    static pl_runner_util& instance();

    prepared_source make_compile_failed_prepared_source(
        int exit_code,
        std::string stderr_text
    );

    std::expected<prepared_source, error_code> prepare_source(
        const std::filesystem::path& source_file_path
    );

private:
    pl_runner_util() = default;

    void initialize_if_needed();

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> cpp_compiler_path_;
    std::optional<std::filesystem::path> python_path_;
    std::optional<std::filesystem::path> java_compiler_path_;
    std::optional<std::filesystem::path> java_runtime_path_;
};
