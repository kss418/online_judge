#pragma once
#include "core/error_code.hpp"

#include <vector>
#include <string>
#include <expected>
#include <filesystem>

namespace code_runner{
    using path = std::filesystem::path;

    struct run_result{
        int exit_code_ = 0;
        std::vector<std::string> output_lines_;
        std::string stderr_text_;

        bool is_success() const noexcept { return exit_code_ == 0; }
    };

    std::expected<run_result, error_code> run_cpp(path binary_path, path input_path);
};
