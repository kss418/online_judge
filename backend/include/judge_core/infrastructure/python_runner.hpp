#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"

#include <expected>
#include <filesystem>

namespace python_runner{
    using path = std::filesystem::path;

    std::expected<program_build::runner_build_result, sandbox_error> build(
        const path& source_file_path
    );
}
