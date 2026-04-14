#pragma once

#include "dto/submission_internal_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/repository_error.hpp"

#include <cstdint>
#include <expected>

namespace pqxx{
    class transaction_base;
}

namespace submission_finalize_repository{
    std::expected<void, repository_error> update_submission_status(
        pqxx::transaction_base& transaction,
        const submission_internal_dto::status_update& status_update_value
    );
    std::expected<submission_response_dto::queued_response, repository_error> rejudge_submission(
        pqxx::transaction_base& transaction,
        std::int64_t submission_id
    );
    std::expected<submission_internal_dto::finalize_result, repository_error> finalize_submission(
        pqxx::transaction_base& transaction,
        const submission_internal_dto::finalize_request& finalize_request_value
    );
}
