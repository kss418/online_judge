#pragma once

#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "error/repository_error.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace problem_snapshot_repository{
    struct testcase_data{
        std::int32_t order = 0;
        std::string input;
        std::string output;
    };

    struct manifest{
        std::int64_t problem_id = 0;
        std::int32_t version = 0;
        problem_content_dto::limits limits_value;
        std::vector<testcase_data> testcases;
    };

    std::expected<manifest, repository_error> fetch_manifest(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id,
        std::int32_t version
    );

    std::expected<void, repository_error> publish_current_snapshot(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
}
