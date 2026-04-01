#include "db_service/submission_service.hpp"
#include "db_service/db_service_util.hpp"
#include "db_repository/problem_statistics_repository.hpp"
#include "db_repository/submission_repository.hpp"
#include "db_repository/user_problem_summary_repository.hpp"
#include "db_repository/user_repository.hpp"

#include <string>
#include <utility>

static bool is_queue_empty_error(const error_code& code){
    return code == errno_error::resource_temporarily_unavailable;
}

static bool should_hide_submission_metrics(std::string_view status){
    return status == to_string(submission_status::runtime_error);
}

template <typename submission_metrics_type>
static void hide_submission_metrics_if_needed(submission_metrics_type& value){
    if(should_hide_submission_metrics(value.status)){
        value.elapsed_ms_opt = std::nullopt;
        value.max_rss_kb_opt = std::nullopt;
    }
}

template <typename submission_metrics_type>
static submission_metrics_type sanitize_submission_metrics(submission_metrics_type value){
    hide_submission_metrics_if_needed(value);
    return value;
}

template <typename submission_metrics_type>
static std::vector<submission_metrics_type> sanitize_submission_metrics(
    std::vector<submission_metrics_type> values
){
    for(auto& value : values){
        hide_submission_metrics_if_needed(value);
    }

    return values;
}

static submission_dto::summary_page sanitize_submission_metrics(
    submission_dto::summary_page summary_page_value
){
    summary_page_value.submissions =
        sanitize_submission_metrics(std::move(summary_page_value.submissions));
    return summary_page_value;
}

std::expected<submission_dto::history_list, error_code> submission_service::get_submission_history(
    db_connection& connection,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(error_code::create(http_error::validation_error));
    }

    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<submission_dto::history_list, error_code> {
            return db_service_util::map_repository_error_to_http_error(
                submission_repository::get_submission_history(transaction, submission_id)
            );
        }
    );
}

std::expected<submission_dto::source_detail, error_code> submission_service::get_submission_source(
    db_connection& connection,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(error_code::create(http_error::validation_error));
    }

    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<submission_dto::source_detail, error_code> {
            return db_service_util::map_repository_error_to_http_error(
                submission_repository::get_submission_source(transaction, submission_id)
            );
        }
    );
}

std::expected<submission_dto::detail, error_code> submission_service::get_submission_detail(
    db_connection& connection,
    std::int64_t submission_id
){
    if(submission_id <= 0){
        return std::unexpected(error_code::create(http_error::validation_error));
    }

    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<submission_dto::detail, error_code> {
            auto submission_detail_exp = db_service_util::map_repository_error_to_http_error(
                submission_repository::get_submission_detail(
                    transaction,
                    submission_id
                )
            );
            if(!submission_detail_exp){
                return std::unexpected(submission_detail_exp.error());
            }

            return sanitize_submission_metrics(*submission_detail_exp);
        }
    );
}

std::expected<std::vector<submission_dto::status_snapshot>, error_code>
submission_service::get_submission_status_snapshots(
    db_connection& connection,
    const std::vector<std::int64_t>& submission_ids
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<submission_dto::status_snapshot>, error_code> {
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

std::expected<std::vector<submission_dto::summary>, error_code>
submission_service::get_wa_or_ac_submissions(
    db_connection& connection,
    std::int64_t problem_id
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<std::vector<submission_dto::summary>, error_code> {
            return submission_repository::get_wa_or_ac_submissions(
                transaction,
                problem_id
            );
        }
    );
}

std::expected<submission_dto::created, error_code> submission_service::create_submission(
    db_connection& connection,
    const submission_dto::create_request& create_request_value
){
    problem_dto::reference problem_reference_value{create_request_value.problem_id};

    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<submission_dto::created, error_code> {
            const auto active_submission_ban_exp =
                user_repository::get_active_submission_banned_until(
                    transaction,
                    create_request_value.user_id
                );
            if(!active_submission_ban_exp){
                return std::unexpected(active_submission_ban_exp.error());
            }
            if(active_submission_ban_exp->has_value()){
                return std::unexpected(error_code::create(errno_error::permission_denied));
            }

            const auto create_submission_exp = submission_repository::create_submission(
                transaction,
                create_request_value
            );
            if(!create_submission_exp){
                return std::unexpected(create_submission_exp.error());
            }

            const auto increase_user_problem_submission_count_exp =
                user_problem_summary_repository::increase_submission_count(
                    transaction,
                    create_request_value.user_id,
                    create_request_value.problem_id
                );
            if(!increase_user_problem_submission_count_exp){
                return std::unexpected(increase_user_problem_submission_count_exp.error());
            }

            const auto enqueue_submission_exp = submission_repository::enqueue_submission(
                transaction,
                create_submission_exp->submission_id,
                submission_repository::NORMAL_SUBMISSION_QUEUE_PRIORITY
            );
            if(!enqueue_submission_exp){
                return std::unexpected(enqueue_submission_exp.error());
            }

            const auto increase_submission_count_exp =
                problem_statistics_repository::increase_submission_count(
                    transaction,
                    problem_reference_value
                );
            if(!increase_submission_count_exp){
                return std::unexpected(increase_submission_count_exp.error());
            }

            return *create_submission_exp;
        }
    );
}

std::expected<void, error_code> submission_service::update_submission_status(
    db_connection& connection,
    const submission_dto::status_update& status_update_value
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            return submission_repository::update_submission_status(
                transaction,
                status_update_value
            );
        }
    );
}

std::expected<void, error_code> submission_service::mark_queued(
    db_connection& connection,
    std::int64_t submission_id
){
    const submission_dto::status_update status_update_value =
        submission_dto::make_status_update(
            submission_id,
            submission_status::queued
        );
    return update_submission_status(connection, status_update_value);
}

std::expected<void, error_code> submission_service::mark_judging(
    db_connection& connection,
    std::int64_t submission_id
){
    const submission_dto::status_update status_update_value =
        submission_dto::make_status_update(
            submission_id,
            submission_status::judging
        );
    return update_submission_status(connection, status_update_value);
}

std::expected<void, error_code> submission_service::rejudge(
    db_connection& connection,
    std::int64_t submission_id
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            return submission_repository::rejudge_submission(
                transaction,
                submission_id
            );
        }
    );
}

std::expected<void, error_code> submission_service::rejudge_problem(
    db_connection& connection,
    std::int64_t problem_id
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto submission_values_exp =
                submission_repository::get_wa_or_ac_submissions(
                    transaction,
                    problem_id
                );
            if(!submission_values_exp){
                return std::unexpected(submission_values_exp.error());
            }

            for(const auto& submission_value : *submission_values_exp){
                const auto rejudge_submission_exp =
                    submission_repository::rejudge_submission(
                    transaction,
                    submission_value.submission_id
                );
                if(!rejudge_submission_exp){
                    return std::unexpected(rejudge_submission_exp.error());
                }
            }

            return {};
        }
    );
}

std::expected<std::optional<submission_dto::queued_submission>, error_code>
submission_service::lease_submission(
    db_connection& connection,
    const submission_dto::lease_request& lease_request_value
){
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction)
            -> std::expected<std::optional<submission_dto::queued_submission>, error_code> {
            auto lease_submission_exp = submission_repository::lease_submission(
                transaction,
                lease_request_value
            );
            if(!lease_submission_exp){
                if(is_queue_empty_error(lease_submission_exp.error())){
                    return std::optional<submission_dto::queued_submission>{};
                }

                return std::unexpected(lease_submission_exp.error());
            }

            return std::optional<submission_dto::queued_submission>{
                std::move(*lease_submission_exp)
            };
        }
    );
}

std::expected<void, error_code> submission_service::requeue_submission_immediately(
    db_connection& connection,
    std::int64_t submission_id,
    std::optional<std::string> reason_opt
){
    const std::optional<std::string> queued_reason_opt = std::move(reason_opt);
    return db_service_util::with_retry_write_transaction(
        connection,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const submission_dto::status_update status_update_value =
                submission_dto::make_status_update(
                    submission_id,
                    submission_status::queued,
                    queued_reason_opt
                );
            const auto update_submission_status_exp = submission_repository::update_submission_status(
                transaction,
                status_update_value
            );
            if(!update_submission_status_exp){
                return std::unexpected(update_submission_status_exp.error());
            }

            return submission_repository::release_submission_lease(
                transaction,
                submission_id
            );
        }
    );
}

std::expected<void, error_code> submission_service::finalize_submission(
    db_connection& connection,
    const submission_dto::finalize_request& finalize_request_value
){
    return db_service_util::with_retry_write_transaction(
        connection,
        db_service_util::DB_TRANSACTION_ATTEMPT_COUNT,
        [&](pqxx::work& transaction) -> std::expected<void, error_code> {
            const auto finalize_submission_exp = submission_repository::finalize_submission(
                transaction,
                finalize_request_value
            );
            if(!finalize_submission_exp){
                return std::unexpected(finalize_submission_exp.error());
            }

            problem_dto::reference problem_reference_value{
                finalize_submission_exp->problem_id
            };
            if(finalize_submission_exp->should_increase_accepted_count){
                const auto increase_accepted_count_exp =
                    problem_statistics_repository::increase_accepted_count(
                        transaction,
                        problem_reference_value
                    );
                if(!increase_accepted_count_exp){
                    return std::unexpected(increase_accepted_count_exp.error());
                }
            }

            return {};
        }
    );
}

std::expected<submission_dto::summary_page, error_code>
submission_service::list_submissions(
    db_connection& connection,
    const submission_dto::list_filter& filter_value,
    std::optional<std::int64_t> viewer_user_id_opt
){
    return db_service_util::with_retry_read_transaction(
        connection,
        [&](pqxx::read_transaction& transaction)
            -> std::expected<submission_dto::summary_page, error_code> {
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
