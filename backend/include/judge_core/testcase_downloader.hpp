#pragma once

#include "common/db_connection.hpp"
#include "error/judge_error.hpp"
#include "judge_core/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>

namespace testcase_downloader{
    std::expected<testcase_snapshot, judge_error> ensure_testcase_snapshot(
        db_connection& connection,
        std::int64_t problem_id
    );
}
