#pragma once

#include "dto/submission_domain_dto.hpp"
#include "dto/submission_internal_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/submission_decision.hpp"
#include "judge_core/gateway/judge_submission_facade.hpp"

#include <expected>
#include <optional>
#include <utility>
#include <variant>

class build_bundle;

class submission_lifecycle{
public:
    struct finalize_command{
        submission_internal_dto::finalize_request request;
    };

    enum class retry_directive{
        requeue_immediately,
        finalize_as_infra_failure
    };

    struct infra_failure_report{
        judge_error error = judge_error::internal;
        retry_directive retry = retry_directive::finalize_as_infra_failure;
    };

    class submission_completion{
    public:
        submission_completion(const submission_completion&) = default;
        submission_completion& operator=(const submission_completion&) = default;
        submission_completion(submission_completion&&) noexcept = default;
        submission_completion& operator=(submission_completion&&) noexcept =
            default;
        ~submission_completion() = default;

    private:
        using storage_type =
            std::variant<finalize_command, infra_failure_report>;

        explicit submission_completion(finalize_command finalize_command_value) :
            storage_(std::move(finalize_command_value)){}

        explicit submission_completion(
            infra_failure_report infra_failure_report_value
        ) :
            storage_(std::move(infra_failure_report_value)){}

        storage_type storage_;

        friend class submission_lifecycle;
    };

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
    std::optional<submission_completion> apply_build_policy(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const build_bundle& build_result_value
    ) const;
    submission_completion make_decision_completion(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const submission_decision& submission_decision_value
    ) const;
    submission_completion make_infra_failure_completion(
        const judge_error& error_value
    ) const;
    std::expected<void, judge_error> apply_completion(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const submission_completion& submission_completion_value
    );

private:
    explicit submission_lifecycle(
        judge_submission_facade judge_submission_facade_value
    );

    std::expected<void, judge_error> apply_finalize_command(
        const finalize_command& finalize_command_value
    );
    std::expected<void, judge_error> apply_infra_failure_report(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const infra_failure_report& infra_failure_report_value
    );
    std::expected<void, judge_error> requeue_submission(
        const submission_domain_dto::leased_submission& leased_submission_value,
        const judge_error& error_value
    );
    retry_directive decide_retry_directive(const judge_error& error_value) const;

    judge_submission_facade judge_submission_facade_;
};
