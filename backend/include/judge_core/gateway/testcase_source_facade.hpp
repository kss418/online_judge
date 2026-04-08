#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_content_dto.hpp"
#include "error/judge_error.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

class testcase_source_facade{
public:
    struct testcase_data{
        std::int32_t order = 0;
        std::string input;
        std::string output;
    };

    struct problem_snapshot_manifest{
        std::int64_t problem_id = 0;
        std::int32_t version = 0;
        problem_content_dto::limits limits_value;
        std::vector<testcase_data> testcases;
    };

    static std::expected<testcase_source_facade, judge_error> create(
        const db_connection_config& db_config
    );

    testcase_source_facade(testcase_source_facade&& other) noexcept;
    testcase_source_facade& operator=(testcase_source_facade&& other) noexcept;
    ~testcase_source_facade();

    testcase_source_facade(const testcase_source_facade&) = delete;
    testcase_source_facade& operator=(const testcase_source_facade&) = delete;

    std::expected<problem_snapshot_manifest, judge_error> fetch_manifest(
        std::int64_t problem_id,
        std::int32_t version
    );

private:
    explicit testcase_source_facade(db_connection db_connection_value);

    db_connection db_connection_;
};
