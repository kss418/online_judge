#pragma once

#include "error/judge_error.hpp"
#include "judge_core/gateway/judge_submission_facade.hpp"
#include "judge_core/types/execution_report.hpp"
#include "judge_core/types/judge_result.hpp"

#include <cstdint>
#include <expected>
#include <string>

class submission_lifecycle{
public:
    static std::expected<submission_lifecycle, judge_error> create(
        judge_submission_facade judge_submission_facade_value
    );

    submission_lifecycle(submission_lifecycle&& other) noexcept;
    submission_lifecycle& operator=(submission_lifecycle&& other) noexcept;
    ~submission_lifecycle();

    submission_lifecycle(const submission_lifecycle&) = delete;
    submission_lifecycle& operator=(const submission_lifecycle&) = delete;

    std::expected<void, judge_error> mark_judging(std::int64_t submission_id);
    std::expected<void, judge_error> finalize_submission(
        std::int64_t submission_id,
        judge_result result,
        const execution_report::batch& execution_report_value
    );
    std::expected<void, judge_error> requeue_submission(
        std::int64_t submission_id,
        std::string reason
    );

private:
    explicit submission_lifecycle(
        judge_submission_facade judge_submission_facade_value
    );

    judge_submission_facade judge_submission_facade_;
};
