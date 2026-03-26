#pragma once
#include "common/error_code.hpp"
#include "common/submission_status.hpp"
#include "dto/submission_dto.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace submission_util{
    inline constexpr std::int16_t NORMAL_SUBMISSION_QUEUE_PRIORITY = 100;
    inline constexpr std::int16_t REJUDGE_SUBMISSION_QUEUE_PRIORITY = 0;

    std::expected<submission_dto::history_list, error_code> get_submission_history(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );
    std::expected<submission_dto::source_detail, error_code> get_submission_source(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );

    std::expected<submission_dto::detail, error_code> get_submission_detail(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );
    std::expected<std::vector<submission_dto::summary>, error_code> get_wa_or_ac_submissions(
        pqxx::transaction_base& transaction,
        std::int64_t problem_id
    );

    std::expected<submission_status, error_code> get_submission_status(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );

    std::expected<submission_dto::created, error_code> create_submission(
        pqxx::transaction_base& transaction,
        const submission_dto::create_request& create_request_value
    );

    std::expected<void, error_code> enqueue_submission(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id,
        std::int16_t priority = NORMAL_SUBMISSION_QUEUE_PRIORITY
    );

    std::expected<void, error_code> update_submission_status(
        pqxx::transaction_base& transaction,
        const submission_dto::status_update& status_update_value
    );

    std::expected<void, error_code> clear_submission_result(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );

    std::expected<void, error_code> decrease_accepted_count_if_submission_accepted(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );

    std::expected<void, error_code> rejudge_submission(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );

    std::expected<submission_dto::queued_submission, error_code> lease_submission(
        pqxx::transaction_base& transaction,
        const submission_dto::lease_request& lease_request_value
    );

    std::expected<submission_dto::finalize_result, error_code> finalize_submission(
        pqxx::transaction_base& transaction,
        const submission_dto::finalize_request& finalize_request_value
    );

    std::expected<std::vector<submission_dto::summary>, error_code> list_submissions(
        pqxx::transaction_base& transaction,
        const submission_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
}
