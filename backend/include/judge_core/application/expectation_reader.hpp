#pragma once

#include "error/judge_error.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"
#include "judge_core/types/judge_expectation.hpp"

#include <expected>

class expectation_reader{
public:
    std::expected<judge_expectation, judge_error> read(
        const testcase_snapshot& testcase_snapshot_value
    ) const;
};
