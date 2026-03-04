#pragma once
#include "common/error_code.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
class transaction_base;
}

namespace problem_service_utility{
std::expected<bool, error_code> exists_problem(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<void, error_code> increase_version(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);
}
