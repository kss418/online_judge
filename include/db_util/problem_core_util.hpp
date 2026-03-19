#pragma once

#include "common/error_code.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace problem_core_util{
    std::expected<bool, error_code> exists_problem(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    );

    std::expected<std::int32_t, error_code> get_version(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    );

    std::expected<std::int64_t, error_code> create_problem(
        pqxx::transaction_base& transaction
    );

    std::expected<problem_dto::limits, error_code> get_limits(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    );

    std::expected<void, error_code> set_limits(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id,
        const problem_dto::limits& limits_value
    );

    std::expected<void, error_code> increase_version(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    );
}
