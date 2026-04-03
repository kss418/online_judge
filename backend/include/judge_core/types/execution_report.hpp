#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace execution_report{
    struct testcase_execution{
        int exit_code = 0;
        std::string stdout_text;
        std::string stderr_text;
        std::int64_t max_rss_kb = 0;
        std::int64_t elapsed_ms = 0;
        bool time_limit_exceeded = false;
        bool memory_limit_exceeded = false;
        bool output_exceeded = false;
    };

    struct batch{
        std::vector<testcase_execution> executions;
        bool compile_failed = false;
        std::int32_t testcase_count = 0;
    };
}
