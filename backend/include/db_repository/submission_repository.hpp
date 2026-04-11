#pragma once
#include "error/repository_error.hpp"
#include "common/submission_status.hpp"
#include "dto/submission_domain_dto.hpp"
#include "dto/submission_internal_dto.hpp"
#include "dto/submission_request_dto.hpp"
#include "dto/submission_response_dto.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace submission_repository{
    struct locked_submission_context{
        std::int64_t user_id = 0;
        std::int64_t problem_id = 0;
        std::int32_t problem_version = 0;
        submission_status status = submission_status::queued;
    };

    inline constexpr std::int16_t NORMAL_SUBMISSION_QUEUE_PRIORITY = 100;
    inline constexpr std::int16_t REJUDGE_SUBMISSION_QUEUE_PRIORITY = 0;

    std::expected<submission_response_dto::history_list, repository_error> get_submission_history(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );
    std::expected<submission_response_dto::source_detail, repository_error> get_submission_source(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );

    std::expected<submission_response_dto::detail, repository_error> get_submission_detail(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );
    std::expected<std::vector<submission_response_dto::status_snapshot>, repository_error>
    get_submission_status_snapshots(
        pqxx::transaction_base& transaction,
        const std::vector<std::int64_t>& submission_ids
    );
    std::expected<std::vector<submission_response_dto::summary>, repository_error>
    get_wa_or_ac_submissions(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    );

    std::expected<submission_status, repository_error> get_submission_status(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );
    std::expected<locked_submission_context, repository_error> get_locked_submission_context(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );
    std::expected<void, repository_error> persist_submission_status_transition(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        submission_status from_status,
        submission_status to_status,
        const std::optional<std::string>& reason_opt
    );

    std::expected<submission_response_dto::queued_response, repository_error> create_submission(
        pqxx::transaction_base& transaction,
        const submission_internal_dto::create_submission_command& create_request_value
    );

    std::expected<void, repository_error> enqueue_submission(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        std::int16_t priority = NORMAL_SUBMISSION_QUEUE_PRIORITY
    );

    std::expected<void, repository_error> update_submission_status(
        pqxx::transaction_base& transaction,
        const submission_internal_dto::status_update& status_update_value
    );

    std::expected<void, repository_error> clear_submission_result(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );

    std::expected<submission_response_dto::queued_response, repository_error> rejudge_submission(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
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

    std::expected<submission_internal_dto::finalize_result, repository_error> finalize_submission(
        pqxx::transaction_base& transaction,
        const submission_internal_dto::finalize_request& finalize_request_value
    );

    std::expected<submission_response_dto::summary_page, repository_error> list_submissions(
        pqxx::transaction_base& transaction,
        const submission_request_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
}
