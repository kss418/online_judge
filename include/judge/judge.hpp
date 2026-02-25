#pragma once
#include "judge/judge_utility.hpp"
#include "core/error_code.hpp"

#include <filesystem>

namespace judge{
    using path = std::filesystem::path;
    std::expected <std::pair<judge_result, std::string>, error_code> judge_cpp(
        path source_path, path input_path, path answer_path, path compiler_path
    );
};