#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/submission_decision.hpp"
#include "judge_core/gateway/judge_submission_facade.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <variant>

class build_bundle;

class submission_lifecycle{
public:
    using build_policy_outcome = std::variant<
        std::monostate,
        submission_decision,
        submission_dto::finalize_request,
        judge_error
    >;

    using completion_outcome = std::variant<
        submission_decision,
        submission_dto::finalize_request,
        judge_error
    >;

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
        completion_outcome submission_outcome_value
    );
    build_policy_outcome apply_build_policy(
        const submission_dto::leased_submission& leased_submission_value,
        const build_bundle& build_result_value
    ) const;

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
    std::expected<void, judge_error> finalize_direct_submission(
        const submission_dto::finalize_request& finalize_request_value
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
