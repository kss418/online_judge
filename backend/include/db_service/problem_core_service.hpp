#pragma once

#include "error/service_error.hpp"
#include "common/db_connection.hpp"
#include "dto/problem_dto.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace problem_core_service{
    inline constexpr std::int32_t INITIAL_MEMORY_LIMIT_MB = 256;
    inline constexpr std::int32_t INITIAL_TIME_LIMIT_MS = 1000;

    std::expected<void, service_error> ensure_problem_exists(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );

    std::expected<problem_dto::title, service_error> get_title(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
    
    std::expected<problem_dto::version, service_error> get_version(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<std::optional<std::string>, service_error> get_user_problem_state(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        std::int64_t user_id
    );
    std::expected<problem_dto::detail, service_error> get_problem_detail(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );

    std::expected<problem_dto::created, service_error> create_problem(
        db_connection& connection,
        const problem_dto::create_request& create_request_value
    );
    std::expected<problem_dto::mutation_result, service_error> update_problem(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value,
        const problem_dto::update_request& update_request_value
    );
    std::expected<void, service_error> delete_problem(
        db_connection& connection,
        const problem_dto::reference& problem_reference_value
    );
    std::expected<std::vector<problem_dto::summary>, service_error> list_problems(
        db_connection& connection,
        const problem_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
    std::expected<std::int64_t, service_error> count_problems(
        db_connection& connection,
        const problem_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );

    std::expected<std::vector<problem_dto::summary>, service_error> list_user_solved_problems(
        db_connection& connection,
        std::int64_t user_id,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
    std::expected<std::vector<problem_dto::summary>, service_error> list_user_wrong_problems(
        db_connection& connection,
        std::int64_t user_id,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
}
