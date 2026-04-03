#pragma once

#include <filesystem>

struct toolchain_config{
    std::filesystem::path cpp_compiler_path;
    std::filesystem::path python_path;
    std::filesystem::path java_compiler_path;
    std::filesystem::path java_runtime_path;
};
