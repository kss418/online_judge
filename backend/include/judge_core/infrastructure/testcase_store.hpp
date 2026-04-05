#pragma once

#include "common/temp_dir.hpp"
#include "dto/problem_content_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/types/testcase_snapshot.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>

class testcase_store{
public:
    struct staging_area{
        temp_dir temp_directory;

        const std::filesystem::path& path() const noexcept{
            return temp_directory.get_path();
        }
    };

    static std::expected<testcase_store, judge_error> create(
        std::filesystem::path testcase_root_path
    );

    testcase_store(testcase_store&& other) noexcept;
    testcase_store& operator=(testcase_store&& other) noexcept;
    ~testcase_store();

    testcase_store(const testcase_store&) = delete;
    testcase_store& operator=(const testcase_store&) = delete;

    std::expected<bool, judge_error> has_version(
        std::int64_t problem_id,
        std::int32_t version
    ) const;

    std::expected<staging_area, judge_error> create_staging_area(
        std::int64_t problem_id,
        std::int32_t version
    ) const;

    std::expected<void, judge_error> write_testcase(
        const staging_area& staging_area_value,
        std::int32_t order,
        std::string_view input,
        std::string_view output
    ) const;

    std::expected<void, judge_error> write_problem_limits(
        const staging_area& staging_area_value,
        const problem_content_dto::limits& problem_limits_value
    ) const;

    std::expected<void, judge_error> publish_version_directory(
        const staging_area& staging_area_value,
        std::int64_t problem_id,
        std::int32_t version
    ) const;

    std::expected<testcase_snapshot, judge_error> load_snapshot(
        std::int64_t problem_id,
        std::int32_t version
    ) const;

private:
    explicit testcase_store(std::filesystem::path testcase_root_path);

    std::filesystem::path testcase_root_path_;
};
