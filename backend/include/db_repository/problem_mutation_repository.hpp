#pragma once

#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"
#include "error/repository_error.hpp"

#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace problem_mutation_repository{
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
    std::expected<void, repository_error> set_limits(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value,
        const problem_content_dto::limits& limits_value
    );
}
