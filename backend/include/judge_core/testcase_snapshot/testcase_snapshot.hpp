#pragma once

#include "dto/problem_content_dto.hpp"
#include "error/io_error.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <utility>


struct testcase_snapshot{
    static testcase_snapshot make(
        std::int64_t problem_id,
        std::int32_t version,
        std::filesystem::path directory_path,
        std::int32_t testcase_count,
        problem_content_dto::limits limits_value
    ){
        testcase_snapshot testcase_snapshot_value;
        testcase_snapshot_value.problem_id = problem_id;
        testcase_snapshot_value.version = version;
        testcase_snapshot_value.directory_path = std::move(directory_path);
        testcase_snapshot_value.testcase_count = testcase_count;
        testcase_snapshot_value.limits_value = std::move(limits_value);
        return testcase_snapshot_value;
    }

    std::int32_t case_count() const noexcept;

    std::expected<void, io_error> validate() const;

    std::expected<std::filesystem::path, io_error> input_path(
        std::int32_t order
    ) const;

    std::expected<std::filesystem::path, io_error> output_path(
        std::int32_t order
    ) const;

    std::int64_t problem_id = 0;
    std::int32_t version = 0;
    std::filesystem::path directory_path;
    std::int32_t testcase_count = 0;
    problem_content_dto::limits limits_value{};
};
