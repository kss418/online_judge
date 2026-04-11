#pragma once

#include "common/submission_status.hpp"
#include "dto/submission_domain_dto.hpp"
#include "dto/submission_response_dto.hpp"
#include "error/repository_error.hpp"

#include <cstddef>
#include <expected>
#include <vector>

namespace pqxx{
    class result;
    class row;
}

namespace submission_row_mapper{
    submission_response_dto::history map_history_row(const pqxx::row& submission_history_row);
    submission_response_dto::history_list map_history_result(
        const pqxx::result& submission_history_result
    );
    submission_response_dto::source_detail map_source_detail_row(
        const pqxx::row& submission_source_row
    );
    submission_response_dto::detail map_detail_row(const pqxx::row& submission_detail_row);
    submission_response_dto::status_snapshot map_status_snapshot_row(
        const pqxx::row& submission_status_row
    );
    std::vector<submission_response_dto::status_snapshot> map_status_snapshot_result(
        const pqxx::result& submission_status_result
    );
    std::expected<submission_status, repository_error> map_submission_status_row(
        const pqxx::row& submission_row,
        std::size_t status_column_index
    );
    submission_response_dto::summary map_summary_row(const pqxx::row& submission_summary_row);
    std::vector<submission_response_dto::summary> map_summary_result(
        const pqxx::result& submission_summary_result
    );
    submission_domain_dto::leased_submission map_leased_submission_row(
        const pqxx::row& submission_queue_row
    );
}
