#pragma once

#include "error/judge_error.hpp"
#include "judge_core/infrastructure/build_dispatcher.hpp"
#include "judge_core/types/compile_failure.hpp"
#include "judge_core/types/runnable_program.hpp"

#include <expected>
#include <filesystem>
#include <variant>

class submission_builder{
public:
    using build_result = std::variant<runnable_program, compile_failure>;

    static std::expected<submission_builder, judge_error> create();

    explicit submission_builder(build_dispatcher build_dispatcher_value);

    submission_builder(submission_builder&& other) noexcept;
    submission_builder& operator=(submission_builder&& other) noexcept;
    ~submission_builder();

    submission_builder(const submission_builder&) = delete;
    submission_builder& operator=(const submission_builder&) = delete;

    std::expected<build_result, judge_error> build(
        const std::filesystem::path& source_file_path
    );

private:
    build_dispatcher build_dispatcher_;
};
