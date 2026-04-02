#pragma once

#include "error/service_error.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_content_dto.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>

namespace problem_statistics_service{
    std::expected<problem_content_dto::statistics, service_error> get_statistics(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
}
