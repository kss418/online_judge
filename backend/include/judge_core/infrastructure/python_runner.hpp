#pragma once

#include <filesystem>
#include <memory>

class program_handler;

std::unique_ptr<program_handler> make_python_program_handler(
    std::filesystem::path python_path
);
