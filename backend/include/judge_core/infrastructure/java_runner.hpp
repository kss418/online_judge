#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"
#include "judge_core/infrastructure/sandbox_runner.hpp"

#include <expected>
#include <filesystem>

namespace java_runner{
    using path = std::filesystem::path;

    std::expected<program_build::build_artifact, sandbox_error> build(
        const path& source_file_path,
        const path& java_compiler_path,
        const sandbox_runner::run_options& compile_run_options_value
    );
}
