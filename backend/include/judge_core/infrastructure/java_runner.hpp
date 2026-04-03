#pragma once

#include <filesystem>
#include <memory>

class program_handler;

std::unique_ptr<program_handler> make_java_program_handler(
    std::filesystem::path java_compiler_path,
    std::filesystem::path java_runtime_path
);
