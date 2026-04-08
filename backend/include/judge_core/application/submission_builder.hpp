#pragma once

#include "dto/submission_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/application/build_bundle.hpp"
#include "judge_core/infrastructure/build_dispatcher.hpp"

#include <expected>
#include <filesystem>

class submission_builder{
public:
    static std::expected<submission_builder, judge_error> create();

    explicit submission_builder(build_dispatcher build_dispatcher_value);

    submission_builder(submission_builder&& other) noexcept;
    submission_builder& operator=(submission_builder&& other) noexcept;
    ~submission_builder();

    submission_builder(const submission_builder&) = delete;
    submission_builder& operator=(const submission_builder&) = delete;

    build_bundle build(
        const std::filesystem::path& source_file_path
    );

private:
    build_dispatcher build_dispatcher_;
};
