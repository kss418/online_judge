#pragma once

#include "dto/submission_internal_dto.hpp"
#include "error/judge_error.hpp"

#include <utility>
#include <variant>

class submission_completion{
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

    submission_completion(const submission_completion&) = default;
    submission_completion& operator=(const submission_completion&) = default;
    submission_completion(submission_completion&&) noexcept = default;
    submission_completion& operator=(submission_completion&&) noexcept = default;
    ~submission_completion() = default;

    static submission_completion from_finalize_command(
        finalize_command finalize_command_value
    ){
        return submission_completion(std::move(finalize_command_value));
    }

    static submission_completion from_infra_failure_report(
        infra_failure_report infra_failure_report_value
    ){
        return submission_completion(std::move(infra_failure_report_value));
    }

    const finalize_command* finalize_command_opt() const{
        return std::get_if<finalize_command>(&storage_);
    }

    const infra_failure_report* infra_failure_report_opt() const{
        return std::get_if<infra_failure_report>(&storage_);
    }

private:
    using storage_type = std::variant<finalize_command, infra_failure_report>;

    explicit submission_completion(finalize_command finalize_command_value) :
        storage_(std::move(finalize_command_value)){}

    explicit submission_completion(
        infra_failure_report infra_failure_report_value
    ) :
        storage_(std::move(infra_failure_report_value)){}

    storage_type storage_;
};
