#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"

#include <expected>
#include <mutex>
#include <optional>

class program_builder{
public:
    static program_builder& instance();

    std::expected<program_build::build_artifact, sandbox_error> build_source(
        const std::filesystem::path& source_file_path
    );

private:
    program_builder() = default;
    void initialize_if_needed();

    std::mutex initialize_mutex_;
    std::optional<std::filesystem::path> cpp_compiler_path_;
    std::optional<std::filesystem::path> java_compiler_path_;
};
