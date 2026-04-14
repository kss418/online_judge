#pragma once

#include "dto/problem_dto.hpp"
#include "error/repository_error.hpp"

#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace problem_version_repository{
    std::expected<problem_dto::version, repository_error> increase_version(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
}
