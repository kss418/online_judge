#pragma once

#include "common/error_code.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
class transaction_base;
}

namespace problem_util{
std::expected<bool, error_code> exists_problem(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<void, error_code> ensure_statement_row(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<void, error_code> increase_version(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<std::int32_t, error_code> increase_sample_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<std::int32_t, error_code> decrease_sample_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<std::int64_t, error_code> create_sample(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::sample& sample_value
);

std::expected<void, error_code> set_sample(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::sample& sample_value
);

std::expected<void, error_code> delete_sample(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::sample& sample_value
);
}
