#pragma once

#include "dto/submission_domain_dto.hpp"
#include "dto/submission_internal_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/repository_error.hpp"

#include <cstdint>
#include <expected>
#include <optional>

namespace pqxx{
    class transaction_base;
}

namespace submission_queue_repository{
    inline constexpr std::int16_t NORMAL_SUBMISSION_QUEUE_PRIORITY = 100;
    inline constexpr std::int16_t REJUDGE_SUBMISSION_QUEUE_PRIORITY = 0;

    std::expected<submission_response_dto::queued_response, repository_error> create_submission(
        pqxx::transaction_base& transaction,
        const submission_internal_dto::create_submission_command& create_request_value
    );
    std::expected<void, repository_error> enqueue_submission(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        std::int16_t priority = NORMAL_SUBMISSION_QUEUE_PRIORITY
    );
    std::expected<std::optional<submission_domain_dto::leased_submission>, repository_error>
    lease_submission(
        pqxx::transaction_base& transaction,
        const submission_internal_dto::lease_request& lease_request_value
    );
    std::expected<void, repository_error> release_submission_lease(
        pqxx::transaction_base& transaction,
        const submission_domain_dto::leased_submission& leased_submission_value
    );
}
