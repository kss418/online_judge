#pragma once

#include "dto/submission_request_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/repository_error.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

namespace pqxx{
    class transaction_base;
}

namespace submission_read_repository{
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
    std::expected<submission_response_dto::summary_page, repository_error> list_submissions(
        pqxx::transaction_base& transaction,
        const submission_request_dto::list_filter& filter_value,
        std::optional<std::int64_t> viewer_user_id_opt = std::nullopt
    );
}
