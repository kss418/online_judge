#pragma once

#include "error/io_error.hpp"
#include "judge_core/judge_expectation.hpp"
#include "judge_core/testcase_snapshot.hpp"

#include <expected>

namespace judge_expectation_loader{
    std::expected<judge_expectation, io_error> load(
        const testcase_snapshot& testcase_snapshot_value
    );
}
