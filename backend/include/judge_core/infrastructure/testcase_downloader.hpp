#pragma once

#include "common/db_connection.hpp"
#include "error/judge_error.hpp"
#include "judge_core/types/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>

namespace testcase_downloader{
    std::expected<testcase_snapshot, judge_error> ensure_testcase_snapshot(
        db_connection& connection,
        const std::filesystem::path& testcase_root_path,
        std::int64_t problem_id
    );
}
