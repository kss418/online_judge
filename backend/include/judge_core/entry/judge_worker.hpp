#pragma once

#include "error/judge_error.hpp"
#include "judge_core/entry/submission_processor.hpp"
#include "judge_core/gateway/judge_queue_facade.hpp"
#include "judge_core/infrastructure/judge_runtime_registry.hpp"

#include <chrono>
#include <expected>
#include <memory>

class judge_worker{
public:
    struct dependencies{
        judge_queue_facade judge_queue_facade_value;
        submission_processor submission_processor_value;
        std::shared_ptr<judge_runtime_registry> judge_runtime_registry_value;
    };

    static std::expected<judge_worker, judge_error> create(
        dependencies dependencies_value
    );

    std::expected<void, judge_error> run();

private:
    judge_worker(
        judge_queue_facade judge_queue_facade_value,
        submission_processor submission_processor_value,
        std::shared_ptr<judge_runtime_registry> judge_runtime_registry
    );
    static constexpr std::chrono::seconds LEASE_DURATION{300};
    static constexpr std::chrono::milliseconds NOTIFICATION_WAIT_TIMEOUT{30000};

    judge_queue_facade judge_queue_facade_;
    submission_processor submission_processor_;
    std::shared_ptr<judge_runtime_registry> judge_runtime_registry_;
};
