#pragma once

#include "common/error_code.hpp"
#include "db/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace problem_content_service{
    std::expected<std::int32_t, error_code> increase_sample_count(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    );

    std::expected<std::int32_t, error_code> decrease_sample_count(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    );

    std::expected<problem_statement, error_code> get_statement(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<void, error_code> set_statement(
        db_connection& connection,
        std::int64_t problem_id,
        const problem_statement& statement
    );

    std::expected<std::vector<sample>, error_code> list_samples(
        db_connection& connection,
        std::int64_t problem_id
    );

    std::expected<std::int64_t, error_code> create_sample(
        db_connection& connection,
        std::int64_t problem_id,
        const sample& sample_value
    );

    std::expected<void, error_code> set_sample(
        db_connection& connection,
        std::int64_t problem_id,
        const sample& sample_value
    );

    std::expected<void, error_code> delete_sample(
        db_connection& connection,
        std::int64_t problem_id,
        const sample& sample_value
    );
}
