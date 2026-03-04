#pragma once

#include <cstdint>
#include <optional>
#include <string>

struct limits{
    std::int32_t memory_limit_mb = 0;
    std::int32_t time_limit_ms = 0;
};

struct testcase{
    std::int64_t testcase_id = 0;
    std::int32_t testcase_order = 0;
    std::string testcase_input;
    std::string testcase_output;
};

struct problem_statement{
    std::string description;
    std::string input_format;
    std::string output_format;
    std::optional<std::string> note = std::nullopt;
};

struct sample{
    std::int64_t sample_id = 0;
    std::int32_t sample_order = 0;
    std::string sample_input;
    std::string sample_output;
};
