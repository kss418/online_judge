#pragma once
#include "judge_server/core/error_code.hpp"
#include "judge_server/judge/judge_utility.hpp"

#include <vector>
#include <string>
#include <filesystem>

namespace checker{
    using path = std::filesystem::path;
    std::expected <judge_result, error_code> check(std::vector <std::string> output, const path& answer_path);
}
