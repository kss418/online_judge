#pragma once

#include "common/error_code.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace problem_core_util{
    std::expected<problem_dto::existence, error_code> exists_problem(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::title, error_code> get_title(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::version, error_code> get_version(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<std::optional<std::string>, error_code> get_user_problem_state(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        std::int64_t user_id
    );

    std::expected<problem_dto::created, error_code> create_problem(
        pqxx::transaction_base& transaction,
        const problem_dto::create_request& create_request_value
    );
    std::expected<void, error_code> set_title(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::title& title_value
    );
    std::expected<void, error_code> delete_problem(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<std::vector<problem_dto::summary>, error_code> list_problems(
        pqxx::transaction_base& transaction,
        const problem_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );

    std::expected<problem_content_dto::limits, error_code> get_limits(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, error_code> set_limits(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::limits& limits_value
    );

    std::expected<void, error_code> increase_version(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
}
