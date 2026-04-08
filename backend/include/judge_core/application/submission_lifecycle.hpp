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
        const submission_dto::leased_submission& leased_submission_value
    );
    std::expected<void, judge_error> complete(
        const submission_dto::leased_submission& leased_submission_value,
        std::expected<submission_decision, judge_error> submission_outcome_value
    );

private:
    enum class retry_directive{
        requeue_immediately,
        finalize_as_infra_failure
    };

    explicit submission_lifecycle(
        judge_submission_facade judge_submission_facade_value
    );

    std::expected<void, judge_error> finalize_judged_submission(
        const submission_dto::leased_submission& leased_submission_value,
        const submission_decision& submission_decision_value
    );
    std::expected<void, judge_error> handle_infra_failure(
        const submission_dto::leased_submission& leased_submission_value,
        const judge_error& error_value
    );
    std::expected<void, judge_error> requeue_submission(
        const submission_dto::leased_submission& leased_submission_value,
        const judge_error& error_value
    );
    retry_directive decide_retry_directive(const judge_error& error_value) const;

    judge_submission_facade judge_submission_facade_;
};
