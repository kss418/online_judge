#pragma once

#include <filesystem>
#include <memory>

class program_handler;

std::unique_ptr<program_handler> make_cpp_program_handler(
    std::filesystem::path cpp_compiler_path
);
