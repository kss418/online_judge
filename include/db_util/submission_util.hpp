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
    std::expected<submission_dto::source_detail, error_code> get_submission_source(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );

    std::expected<submission_dto::detail, error_code> get_submission_detail(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );

    std::expected<submission_dto::created, error_code> create_submission(
        pqxx::transaction_base& transaction,
        const submission_dto::create_request& create_request_value
    );

    std::expected<void, error_code> update_submission_status(
        pqxx::transaction_base& transaction,
        const submission_dto::status_update& status_update_value
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
        const submission_dto::list_filter& filter_value
    );
}
