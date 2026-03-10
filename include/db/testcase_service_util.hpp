#pragma once

#include "common/error_code.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
class transaction_base;
}

namespace testcase_service_utility{
std::expected<std::int32_t, error_code> increase_testcase_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);

std::expected<std::int32_t, error_code> decrease_testcase_count(
    pqxx::transaction_base& transaction,
    std::int64_t problem_id
);
}
