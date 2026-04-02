#pragma once

#include "common/repository_error.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace problem_statistics_repository{
    std::expected<problem_content_dto::statistics, repository_error> get_statistics(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, repository_error> create_problem_statistics(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, repository_error> increase_submission_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, repository_error> increase_accepted_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, repository_error> decrease_accepted_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
}
