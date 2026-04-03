#pragma once

#include "error/sandbox_error.hpp"
#include "pl_runner/pl_runner_util.hpp"

#include <expected>
#include <filesystem>

namespace cpp_runner{
    using path = std::filesystem::path;

    std::expected<pl_runner_util::build_artifact, sandbox_error> build(
        const path& source_file_path,
        const path& compiler_path
    );
}
