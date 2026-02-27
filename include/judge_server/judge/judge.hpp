#pragma once
#include "judge_server/judge/judge_utility.hpp"
#include "judge_server/core/error_code.hpp"

#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <chrono>

namespace judge{
    using path = std::filesystem::path;

    std::expected <std::pair<judge_result, std::string>, error_code> judge_cpp(
        const path& source_path, const path& input_path, const path& answer_path, const path& compiler_path,
        std::chrono::milliseconds time_limit, std::int64_t memory_limit_mb 
    );
};
