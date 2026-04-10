#pragma once

#include "error/service_error.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace problem_content_service{
    std::expected<problem_content_dto::limits, service_error> get_limits(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, service_error> set_limits(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::limits& limits_value
    );

    std::expected<std::optional<problem_content_dto::statement>, service_error> get_statement(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, service_error> set_statement(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::statement& statement
    );

    std::expected<problem_content_dto::sample, service_error> create_sample(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::sample& sample_value
    );
    std::expected<problem_content_dto::sample, service_error> get_sample(
        db_connection& connection,
        const problem_content_dto::sample_ref& sample_reference_value
    );
    std::expected<std::vector<problem_content_dto::sample>, service_error> list_samples(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<void, service_error> set_sample(
        db_connection& connection,
        const problem_content_dto::sample_ref& sample_reference_value,
        const problem_content_dto::sample& sample_value
    );
    std::expected<problem_content_dto::sample, service_error> set_sample_and_get(
        db_connection& connection,
        const problem_content_dto::sample_ref& sample_reference_value,
        const problem_content_dto::sample& sample_value
    );
    std::expected<void, service_error> delete_sample(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
}
