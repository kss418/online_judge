#pragma once
#include "core/error_code.hpp"
#include "core/unique_fd.hpp"

#include <vector>
#include <string>
#include <expected>
#include <filesystem>

class code_runner{
    using path = std::filesystem::path;
    std::vector <std::string> normalize_output(const std::string& output);
    bool is_blank(char c);
public:
    std::expected <std::vector <std::string>, error_code> run_cpp(path binary_path, path input_path);
};
