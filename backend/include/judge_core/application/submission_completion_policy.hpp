#pragma once

#include "dto/submission_domain_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/submission_completion.hpp"
#include "judge_core/application/submission_decision.hpp"

#include <optional>

class build_bundle;

class submission_completion_policy{
public:
    std::optional<submission_completion> make_build_completion(
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

private:
    submission_completion::retry_directive decide_retry_directive(
        const judge_error& error_value
    ) const;
};
