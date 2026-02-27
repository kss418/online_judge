#pragma once
#include <string>
#include <vector>

enum class judge_result{
    accepted,
    wrong_answer,
    time_limit_excced,
    memory_limit_excced,
    runtime_error,
    compile_error,
    output_exceed,
    invalid_output
};

namespace judge_utility{
    std::vector <std::string> normalize_output(const std::string& output);
    bool is_blank(char c);
};
