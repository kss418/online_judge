#include "db_service/submission_query_service.hpp"

#include "db_service/db_service_util.hpp"
#include "db_repository/submission_repository.hpp"

#include <string_view>
#include <utility>

namespace{
    bool should_hide_submission_metrics(std::string_view status){
        return status == to_string(submission_status::runtime_error);
    }

    template <typename submission_metrics_type>
    void hide_submission_metrics_if_needed(submission_metrics_type& value){
        if(should_hide_submission_metrics(value.status)){
            value.elapsed_ms_opt = std::nullopt;
            value.max_rss_kb_opt = std::nullopt;
        }
    }

    template <typename submission_metrics_type>
    submission_metrics_type sanitize_submission_metrics(submission_metrics_type value){
        hide_submission_metrics_if_needed(value);
        return value;
    }

    template <typename submission_metrics_type>
    std::vector<submission_metrics_type> sanitize_submission_metrics(
        std::vector<submission_metrics_type> values
    ){
        for(auto& value : values){
            hide_submission_metrics_if_needed(value);
        }

        return values;
    }

    submission_response_dto::summary_page sanitize_submission_metrics(
        submission_response_dto::summary_page summary_page_value
    ){
        summary_page_value.submissions =
            sanitize_submission_metrics(std::move(summary_page_value.submissions));
        return summary_page_value;
    }
}

std::expected<submission_response_dto::history_list, service_error>
submission_query_service::get_submission_history(
    db_connection& connection,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<submission_response_dto::history_list, service_error> {
            return submission_repository::get_submission_history(transaction, submission_id);
        }
    );
}

std::expected<submission_response_dto::source_detail, service_error>
submission_query_service::get_submission_source(
    db_connection& connection,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<submission_response_dto::source_detail, service_error> {
            return submission_repository::get_submission_source(transaction, submission_id);
        }
    );
}

std::expected<submission_response_dto::detail, service_error>
submission_query_service::get_submission_detail(
    db_connection& connection,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(service_error::validation_error);
    }

    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<submission_response_dto::detail, service_error> {
            auto submission_detail_exp = submission_repository::get_submission_detail(
                transaction,
                submission_id
            );
            if(!submission_detail_exp){
                return std::unexpected(submission_detail_exp.error());
            }

            return sanitize_submission_metrics(*submission_detail_exp);
        }
    );
}

std::expected<std::vector<submission_response_dto::status_snapshot>, service_error>
submission_query_service::get_submission_status_snapshots(
    db_connection& connection,
    const std::vector<std::int64_t>& submission_ids
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<submission_response_dto::status_snapshot>, service_error> {
            const auto snapshot_values_exp =
                submission_repository::get_submission_status_snapshots(
                    transaction,
                    submission_ids
                );
            if(!snapshot_values_exp){
                return std::unexpected(snapshot_values_exp.error());
            }

            return sanitize_submission_metrics(std::move(*snapshot_values_exp));
        }
    );
}

std::expected<submission_response_dto::summary_page, service_error>
submission_query_service::list_submissions(
    db_connection& connection,
    const submission_request_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    return db_service_util::with_retry_service_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<submission_response_dto::summary_page, service_error> {
            const auto summary_page_exp = submission_repository::list_submissions(
                transaction,
                filter_value,
                viewer_user_id_opt
            );
            if(!summary_page_exp){
                return std::unexpected(summary_page_exp.error());
            }

            return sanitize_submission_metrics(std::move(*summary_page_exp));
        }
    );
}
