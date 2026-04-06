#pragma once

#include "common/db_connection.hpp"
#include "dto/problem_content_dto.hpp"
#include "error/judge_error.hpp"

#include <cstdint>
#include <expected>
#include <string>

class testcase_source_facade{
public:
    struct testcase_data{
        std::int32_t order = 0;
        std::string input;
        std::string output;
    };

    static std::expected<testcase_source_facade, judge_error> create(
        const db_connection_config& db_config
    );

    testcase_source_facade(testcase_source_facade&& other) noexcept;
    testcase_source_facade& operator=(testcase_source_facade&& other) noexcept;
    ~testcase_source_facade();

    testcase_source_facade(const testcase_source_facade&) = delete;
    testcase_source_facade& operator=(const testcase_source_facade&) = delete;

    std::expected<std::int32_t, judge_error> fetch_problem_version(
        std::int64_t problem_id
    );

    std::expected<problem_content_dto::limits, judge_error> fetch_problem_limits(
        std::int64_t problem_id
    );

    std::expected<std::int32_t, judge_error> fetch_testcase_count(
        std::int64_t problem_id
    );

    std::expected<testcase_data, judge_error> fetch_testcase(
        std::int64_t problem_id,
        std::int32_t order
    );

private:
    explicit testcase_source_facade(db_connection db_connection_value);

    db_connection db_connection_;
};
