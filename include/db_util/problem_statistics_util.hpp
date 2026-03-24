#pragma once

#include "common/error_code.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace problem_statistics_util{
    std::expected<problem_content_dto::statistics, error_code> get_statistics(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> create_problem_statistics(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> increase_submission_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> increase_accepted_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> decrease_accepted_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
}
