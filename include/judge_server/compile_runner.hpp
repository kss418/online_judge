#pragma once
#include "common/error_code.hpp"

#include <filesystem>

namespace compile_runner{
    using path = std::filesystem::path;
    struct compile_result{
        int exit_code_;
        std::string stderr_text_;
        bool is_success() const{ return exit_code_ == 0; }
    };

    std::expected <compile_result, error_code> compile_cpp(
        const path& source_path, const path& output_path, const path& compiler_path
    );
};
