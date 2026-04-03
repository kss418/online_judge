#pragma once

#include "dto/problem_content_dto.hpp"
#include "judge_core/infrastructure/testcase_util.hpp"

#include <cstdint>
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

    std::expected<std::filesystem::path, io_error> make_input_path(std::int32_t order) const{
        return testcase_util::make_testcase_input_path(directory_path, order);
    }

    std::expected<std::filesystem::path, io_error> make_output_path(std::int32_t order) const{
        return testcase_util::make_testcase_output_path(directory_path, order);
    }

    std::expected<std::filesystem::path, io_error> make_memory_limit_file_path() const{
        return testcase_util::make_testcase_memory_limit_file_path(directory_path);
    }

    std::expected<std::filesystem::path, io_error> make_time_limit_file_path() const{
        return testcase_util::make_testcase_time_limit_file_path(directory_path);
    }

    std::expected<std::int32_t, io_error> validate_testcase_layout() const{
        return testcase_util::validate_testcase_output(directory_path, testcase_count);
    }

    std::int64_t problem_id = 0;
    std::int32_t version = 0;
    std::filesystem::path directory_path;
    std::int32_t testcase_count = 0;
    problem_content_dto::limits limits_value{};
};
