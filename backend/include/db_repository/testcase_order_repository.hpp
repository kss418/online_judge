#pragma once

#include "dto/problem_dto.hpp"
#include "error/repository_error.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace testcase_order_repository{
    std::expected<void, repository_error> move_testcase(
        pqxx::transaction_base& transaction,
        const problem_dto::testcase_ref& testcase_reference_value,
        std::int32_t target_testcase_order
    );
}
