#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/build_bundle.hpp"
#include "judge_core/application/workspace_session.hpp"
#include "judge_core/infrastructure/build_dispatcher.hpp"

#include <expected>

class submission_builder{
public:
    using build_result = build_bundle;

    struct build_input{
        const submission_dto::queued_submission& queued_submission_value;
        workspace_session& workspace_session_value;
    };

    static std::expected<submission_builder, judge_error> create();

    explicit submission_builder(build_dispatcher build_dispatcher_value);

    submission_builder(submission_builder&& other) noexcept;
    submission_builder& operator=(submission_builder&& other) noexcept;
    ~submission_builder();

    submission_builder(const submission_builder&) = delete;
    submission_builder& operator=(const submission_builder&) = delete;

    std::expected<build_result, judge_error> build(const build_input& build_input_value);

private:
    build_dispatcher build_dispatcher_;
};
