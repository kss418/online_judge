#pragma once

#include "common/error_code.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace pqxx{
class transaction_base;
}

namespace testcase_util{
std::expected<std::int32_t, error_code> increase_tc_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<std::int32_t, error_code> decrease_tc_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<std::vector<problem_dto::tc>, error_code> list_tcs(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<void, error_code> set_tc(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id,
    const problem_dto::tc& tc_value
);

std::expected<void, error_code> delete_tc(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);
}
