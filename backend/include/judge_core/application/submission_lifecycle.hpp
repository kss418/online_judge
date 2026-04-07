#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/submission_decision.hpp"
#include "judge_core/gateway/judge_submission_facade.hpp"

#include <cstdint>
#include <expected>

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

    std::expected<void, judge_error> mark_judging(
        const submission_dto::queued_submission& queued_submission_value
    );
    std::expected<void, judge_error> finalize(
        const submission_dto::queued_submission& queued_submission_value,
        const submission_decision& submission_decision_value
    );
    std::expected<void, judge_error> requeue(
        const submission_dto::queued_submission& queued_submission_value,
        const judge_error& error_value
    );

private:
    explicit submission_lifecycle(
        judge_submission_facade judge_submission_facade_value
    );

    judge_submission_facade judge_submission_facade_;
};
