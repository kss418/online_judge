#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace execution_report{
    struct applied_limits{
        std::int64_t time_limit_ms = 0;
        std::int64_t memory_limit_kb = 0;
        std::int64_t output_limit_bytes = 0;
    };

    struct testcase_execution{
        int exit_code = 0;
        std::optional<int> termination_signal = std::nullopt;
        bool killed_by_wall_clock = false;
        std::string stdout_text;
        std::string stderr_text;
        std::int64_t stdout_bytes = 0;
        std::int64_t stderr_bytes = 0;
        std::int64_t max_rss_kb = 0;
        std::int64_t wall_time_ms = 0;
        std::int64_t cpu_time_ms = 0;
    };

    struct batch{
        std::vector<testcase_execution> executions;
        applied_limits limits;
        bool compile_failed = false;
        std::int32_t testcase_count = 0;
    };
}
