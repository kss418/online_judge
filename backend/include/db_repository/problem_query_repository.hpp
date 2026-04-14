#pragma once

#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "error/repository_error.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace problem_query_repository{
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
}
