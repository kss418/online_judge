#pragma once

#include "error/sandbox_error.hpp"
#include "judge_core/infrastructure/program_build_types.hpp"

#include <expected>
#include <filesystem>

namespace python_artifact_builder{
    using path = std::filesystem::path;

    std::expected<program_build::build_artifact, sandbox_error> build(
        const path& source_file_path
    );
}
