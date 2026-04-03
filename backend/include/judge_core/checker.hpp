#pragma once
#include "judge_core/judge_expectation.hpp"
#include "judge_core/judge_result.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace checker{
    std::vector<std::string> normalize_output(std::string_view output);
    bool check(
        std::string_view output,
        std::string_view expected_output
    );
    judge_result check_all(
        const std::vector<std::string>& output,
        const judge_expectation& judge_expectation_value
    );
}
