#pragma once

#include "dto/problem_dto.hpp"
#include "error/service_error.hpp"

#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace problem_version_publish_service{
    std::expected<void, service_error> publish_current_snapshot(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<problem_dto::version, service_error> increase_version_and_publish_current_snapshot(
        pqxx::transaction_base& transaction,
        const problem_dto::reference& problem_reference_value
    );
}
