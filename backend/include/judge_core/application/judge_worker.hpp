#pragma once

#include "error/judge_error.hpp"
#include "judge_core/application/submission_processor.hpp"

#include <chrono>
#include <expected>

class judge_worker{
public:
    struct dependencies{
        submission_processor submission_processor_value;
    };

    static std::expected<judge_worker, judge_error> create(
        dependencies dependencies_value
    );

    std::expected<void, judge_error> run();

private:
    explicit judge_worker(submission_processor submission_processor_value);
    static constexpr std::chrono::seconds LEASE_DURATION{300};
    static constexpr std::chrono::milliseconds NOTIFICATION_WAIT_TIMEOUT{30000};

    submission_processor submission_processor_;
};
