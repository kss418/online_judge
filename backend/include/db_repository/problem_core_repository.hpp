#pragma once

#include "common/repository_error.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace problem_core_repository{
    std::expected<problem_dto::existence, repository_error> exists_problem(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::title, repository_error> get_title(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::version, repository_error> get_version(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<std::optional<std::string>, repository_error> get_user_problem_state(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        std::int64_t user_id
    );

    std::expected<problem_dto::created, repository_error> create_problem(
        pqxx::transaction_base& transaction,
        const problem_dto::create_request& create_request_value
    );
    std::expected<void, repository_error> set_title(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::title& title_value
    );
    std::expected<void, repository_error> delete_problem(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<std::vector<problem_dto::summary>, repository_error> list_problems(
        pqxx::transaction_base& transaction,
        const problem_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
    std::expected<std::int64_t, repository_error> count_problems(
        pqxx::transaction_base& transaction,
        const problem_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
    std::expected<std::vector<problem_dto::summary>, repository_error> list_user_solved_problems(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
    std::expected<std::vector<problem_dto::summary>, repository_error> list_user_wrong_problems(
        pqxx::transaction_base& transaction,
        std::int64_t user_id,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );

    std::expected<problem_content_dto::limits, repository_error> get_limits(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<void, repository_error> set_limits(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::limits& limits_value
    );

    std::expected<void, repository_error> increase_version(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
}
