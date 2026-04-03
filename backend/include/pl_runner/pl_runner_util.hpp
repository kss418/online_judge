#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/sandbox_runner.hpp"

#include <filesystem>
#include <expected>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

class pl_runner_util{
public:
    enum class source_language{
        cpp,
        python,
        java
    };

    struct build_artifact{
        source_language language_ = source_language::cpp;
        std::filesystem::path entry_host_path_;
        std::string entry_name_;
        std::optional<sandbox_runner::run_result> compile_failed_run_result_;

        bool is_runnable() const noexcept{
            return !compile_failed_run_result_.has_value();
        }
    };

    struct execution_plan{
        std::filesystem::path workspace_host_path_;
        std::vector<std::string> run_command_args_;
        sandbox_runner::mount_profile mount_profile_ =
            sandbox_runner::mount_profile::default_profile;
    };

    static pl_runner_util& instance();

    build_artifact make_compile_failed_artifact(
        source_language language,
        int exit_code,
        std::string stderr_text
    );

    std::expected<build_artifact, sandbox_error> build_source(
        const std::filesystem::path& source_file_path
    );

    std::expected<execution_plan, sandbox_error> make_execution_plan(
        const build_artifact& build_artifact_value
    );

    sandbox_runner::run_options make_compile_run_options(
        const std::filesystem::path& workspace_host_path,
        sandbox_runner::mount_profile mount_profile =
            sandbox_runner::mount_profile::default_profile
    ) const;

private:
    pl_runner_util() = default;

    void initialize_if_needed();

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> cpp_compiler_path_;
    std::optional<std::filesystem::path> python_path_;
    std::optional<std::filesystem::path> java_compiler_path_;
    std::optional<std::filesystem::path> java_runtime_path_;
};
