#pragma once

#include "error/sandbox_error.hpp"
#include "pl_runner/program_build_types.hpp"

#include <expected>
#include <mutex>
#include <optional>

class program_builder{
public:
    static program_builder& instance();

    std::expected<program_build::build_artifact, sandbox_error> build_source(
        const std::filesystem::path& source_file_path
    );

    std::expected<program_build::execution_plan, sandbox_error> make_execution_plan(
        const program_build::build_artifact& build_artifact_value
    );

private:
    program_builder() = default;

    void initialize_if_needed();

    sandbox_runner::run_options make_compile_run_options(
        const std::filesystem::path& workspace_host_path,
        sandbox_runner::mount_profile mount_profile =
            sandbox_runner::mount_profile::default_profile
    ) const;

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> cpp_compiler_path_;
    std::optional<std::filesystem::path> python_path_;
    std::optional<std::filesystem::path> java_compiler_path_;
    std::optional<std::filesystem::path> java_runtime_path_;
};
