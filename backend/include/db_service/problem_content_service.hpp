#pragma once

#include "error/error_code.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <vector>

namespace problem_content_service{
    std::expected<problem_content_dto::limits, error_code> get_limits(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> set_limits(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::limits& limits_value
    );

    std::expected<problem_content_dto::statement, error_code> get_statement(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> set_statement(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::statement& statement
    );

    std::expected<problem_content_dto::sample, error_code> create_sample(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::sample& sample_value
    );
    std::expected<problem_content_dto::sample, error_code> get_sample(
        db_connection& connection,
        const problem_content_dto::sample_ref& sample_reference_value
    );
    std::expected<std::vector<problem_content_dto::sample>, error_code> list_samples(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<void, error_code> set_sample(
        db_connection& connection,
        const problem_content_dto::sample_ref& sample_reference_value,
        const problem_content_dto::sample& sample_value
    );
    std::expected<void, error_code> delete_sample(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
}
