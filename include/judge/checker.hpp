#pragma once
#include "core/error_code.hpp"
#include "judge/judge_utility.hpp"

#include <vector>
#include <string>
#include <filesystem>

namespace checker{
    using path = std::filesystem::path;
    std::expected <judge_result, error_code> check(std::vector <std::string> output, path answer_path);
}
