#pragma once

#include "error/repository_error.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace problem_content_repository{
    std::expected<void, repository_error> ensure_statement_row(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_content_dto::statement, repository_error> get_statement(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, repository_error> set_statement(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::statement& statement_value
    );

    std::expected<std::vector<problem_content_dto::sample>, repository_error> list_samples(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<problem_content_dto::sample, repository_error> get_sample(
        pqxx::transaction_base& transaction,
        const problem_content_dto::sample_ref& sample_reference_value
    );

    std::expected<problem_content_dto::sample_count, repository_error> increase_sample_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_content_dto::sample_count, repository_error> decrease_sample_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_content_dto::sample, repository_error> create_sample(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::sample& sample_value
    );

    std::expected<void, repository_error> set_sample(
        pqxx::transaction_base& transaction,
        const problem_content_dto::sample_ref& sample_reference_value,
        const problem_content_dto::sample& sample_value
    );

    std::expected<void, repository_error> delete_sample(
        pqxx::transaction_base& transaction,
        const problem_content_dto::sample_ref& sample_reference_value
    );
}
