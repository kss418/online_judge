#pragma once

#include "dto/submission_domain_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/submission_completion.hpp"
#include "judge_core/gateway/judge_submission_facade.hpp"

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
        const submission_domain_dto::leased_submission& leased_submission_value
    );
    std::expected<void, judge_error> apply_completion(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const submission_completion& submission_completion_value
    );

private:
    explicit submission_lifecycle(
        judge_submission_facade judge_submission_facade_value
    );

    std::expected<void, judge_error> apply_finalize_command(
        const submission_completion::finalize_command& finalize_command_value
    );
    std::expected<void, judge_error> apply_infra_failure_report(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const submission_completion::infra_failure_report&
            infra_failure_report_value
    );
    std::expected<void, judge_error> requeue_submission(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const judge_error& error_value
    );

    judge_submission_facade judge_submission_facade_;
};
