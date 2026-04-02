#pragma once
#include "error/error_code.hpp"
#include "judge_core/judge_util.hpp"
#include "judge_core/testcase_snapshot.hpp"

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
        const testcase_snapshot& testcase_snapshot_value
    );
}
