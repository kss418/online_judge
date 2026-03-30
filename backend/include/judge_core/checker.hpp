#pragma once
#include "common/error_code.hpp"
#include "judge_core/judge_util.hpp"

#include <cstdint>
#include <filesystem>
#include <expected>
#include <string>
#include <vector>

namespace checker{
    using path = std::filesystem::path;
    std::expected<bool, error_code> check(
        const std::vector<std::string>& output,
        const path& answer_path
    );
    std::expected<judge_result, error_code> check_all(
        const std::vector<std::vector<std::string>>& output,
        const path& testcase_directory_path
    );
}
