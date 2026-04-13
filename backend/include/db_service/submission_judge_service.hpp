#pragma once

#include "common/db_connection.hpp"
#include "dto/submission_domain_dto.hpp"
#include "dto/submission_internal_dto.hpp"
#include "error/service_error.hpp"

#include <expected>
#include <optional>
#include <string>

namespace submission_judge_service{
    std::expected<void, service_error> mark_judging(
        db_connection& connection,
        const submission_domain_dto::leased_submission& leased_submission_value
    );
    std::expected<std::optional<submission_domain_dto::leased_submission>, service_error>
    lease_submission(
        db_connection& connection,
        const submission_internal_dto::lease_request& lease_request_value
    );
    std::expected<void, service_error> requeue_submission_immediately(
        db_connection& connection,
        const submission_domain_dto::leased_submission& leased_submission_value,
        std::optional<std::string> reason_opt = std::nullopt
    );
    std::expected<void, service_error> finalize_submission(
        db_connection& connection,
        const submission_internal_dto::finalize_request& finalize_request_value
    );
}
