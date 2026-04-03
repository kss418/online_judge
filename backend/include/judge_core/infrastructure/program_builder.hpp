#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"

#include <expected>

class program_builder{
public:
    program_builder(
        std::filesystem::path cpp_compiler_path,
        std::filesystem::path java_compiler_path
    );

    std::expected<program_build::build_artifact, sandbox_error> build_source(
        const std::filesystem::path& source_file_path
    );

private:
    std::filesystem::path cpp_compiler_path_;
    std::filesystem::path java_compiler_path_;
};
