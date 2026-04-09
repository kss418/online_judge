#pragma once

#include "dto/problem_content_dto.hpp"

#include <cstdint>

struct snapshot_manifest{
    static constexpr std::int32_t current_schema_version = 1;

    std::int32_t schema_version = current_schema_version;
    std::int64_t problem_id = 0;
    std::int32_t version = 0;
    std::int32_t testcase_count = 0;
    problem_content_dto::limits limits_value{};
};
