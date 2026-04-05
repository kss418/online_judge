#pragma once

#include <filesystem>
#include <string>

enum class runnable_program_language{
    cpp,
    python,
    java
};

struct runnable_program{
    runnable_program_language language = runnable_program_language::cpp;
    std::filesystem::path workspace_host_path;
    std::filesystem::path entry_file_host_path;
    std::string main_class_name;
};
