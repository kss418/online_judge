#pragma once

#include "common/temp_dir.hpp"
#include "dto/problem_content_dto.hpp"
#include "error/judge_error.hpp"
#include "judge_core/testcase_snapshot/testcase_snapshot.hpp"

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

    struct snapshot_manifest{
        static constexpr std::int32_t current_schema_version = 1;

        std::int32_t schema_version = current_schema_version;
        std::int64_t problem_id = 0;
        std::int32_t version = 0;
        std::int32_t testcase_count = 0;
        problem_content_dto::limits limits_value{};
    };

    static std::expected<testcase_store, judge_error> create(
        std::filesystem::path testcase_root_path
    );

    testcase_store(testcase_store&& other) noexcept;
    testcase_store& operator=(testcase_store&& other) noexcept;
    ~testcase_store();

    testcase_store(const testcase_store&) = delete;
    testcase_store& operator=(const testcase_store&) = delete;

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

    std::expected<void, judge_error> write_manifest(
        const staging_area& staging_area_value,
        const snapshot_manifest& manifest_value
    ) const;

    std::expected<void, judge_error> validate_staging_area(
        const staging_area& staging_area_value,
        const snapshot_manifest& manifest_value
    ) const;

    std::expected<void, judge_error> publish_version_directory(
        const staging_area& staging_area_value,
        std::int64_t problem_id,
        std::int32_t version
    ) const;

    std::expected<snapshot_manifest, judge_error> read_manifest(
        std::int64_t problem_id,
        std::int32_t version
    ) const;

    std::expected<bool, judge_error> has_valid_manifest(
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
