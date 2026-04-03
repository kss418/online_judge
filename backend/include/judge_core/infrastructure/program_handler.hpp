#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/execution_plan.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <vector>

class program_handler{
public:
    virtual ~program_handler() = default;

    virtual bool supports_source_file(
        const std::filesystem::path& source_file_path
    ) const = 0;

    virtual program_build::source_language language() const noexcept = 0;

    virtual std::expected<program_build::build_artifact, sandbox_error> build_source(
        const std::filesystem::path& source_file_path
    ) const = 0;

    virtual std::expected<program_launch::execution_plan, sandbox_error>
    make_execution_plan(
        const program_build::build_artifact& build_artifact_value
    ) const = 0;
};

class program_handler_registry{
public:
    explicit program_handler_registry(
        std::vector<std::unique_ptr<program_handler>> handlers
    );

    std::expected<const program_handler*, sandbox_error> find_for_source_file(
        const std::filesystem::path& source_file_path
    ) const;

    std::expected<const program_handler*, sandbox_error> find_for_build_artifact(
        const program_build::build_artifact& build_artifact_value
    ) const;

private:
    std::vector<std::unique_ptr<program_handler>> handlers_;
};
