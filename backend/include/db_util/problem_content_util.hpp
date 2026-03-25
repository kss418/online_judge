#pragma once

#include "common/error_code.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace problem_content_util{
    std::expected<void, error_code> ensure_statement_row(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_content_dto::statement, error_code> get_statement(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> set_statement(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::statement& statement_value
    );

    std::expected<std::vector<problem_content_dto::sample>, error_code> list_samples(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<problem_content_dto::sample, error_code> get_sample(
        pqxx::transaction_base& transaction,
        const problem_content_dto::sample_ref& sample_reference_value
    );

    std::expected<problem_content_dto::sample_count, error_code> increase_sample_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_content_dto::sample_count, error_code> decrease_sample_count(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_content_dto::sample, error_code> create_sample(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::sample& sample_value
    );

    std::expected<void, error_code> set_sample(
        pqxx::transaction_base& transaction,
        const problem_content_dto::sample_ref& sample_reference_value,
        const problem_content_dto::sample& sample_value
    );

    std::expected<void, error_code> delete_sample(
        pqxx::transaction_base& transaction,
        const problem_content_dto::sample_ref& sample_reference_value
    );
}
