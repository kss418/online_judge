#include "db_service/submission_core_service.hpp"
#include "db_util/problem_statistics_util.hpp"
#include "db_util/submission_util.hpp"

#include <pqxx/pqxx>

#include <string>
#include <utility>

static bool is_queue_empty_error(const error_code& code){
    return code == errno_error::resource_temporarily_unavailable;
}

std::expected<std::int64_t, error_code> submission_core_service::create_submission(
    db_connection& connection,
    std::int64_t user_id,
    std::int64_t problem_id,
    const submission_dto::source& source_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(
        user_id <= 0 ||
        problem_id <= 0 ||
        source_value.language.empty() ||
        source_value.source_code.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto create_submission_exp = submission_util::create_submission(
            transaction,
            user_id,
            problem_id,
            source_value
        );
        if(!create_submission_exp){
            return std::unexpected(create_submission_exp.error());
        }

        const auto increase_submission_count_exp = problem_statistics_util::increase_submission_count(
            transaction,
            problem_id
        );
        if(!increase_submission_count_exp){
            return std::unexpected(increase_submission_count_exp.error());
        }

        transaction.commit();
        return create_submission_exp.value();
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> submission_core_service::update_submission_status(
    db_connection& connection,
    std::int64_t submission_id,
    submission_status to_status,
    const std::optional<std::string>& reason_opt
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto update_submission_status_exp = submission_util::update_submission_status(
            transaction,
            submission_id,
            to_status,
            reason_opt
        );
        if(!update_submission_status_exp){
            return std::unexpected(update_submission_status_exp.error());
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::optional<submission_dto::queued_submission>, error_code>
submission_core_service::lease_submission(
    db_connection& connection,
    std::chrono::seconds lease_duration
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        auto lease_submission_exp = submission_util::lease_submission(transaction, lease_duration);
        if(!lease_submission_exp){
            if(is_queue_empty_error(lease_submission_exp.error())){
                return std::optional<submission_dto::queued_submission>{};
            }

            return std::unexpected(lease_submission_exp.error());
        }

        transaction.commit();
        return std::optional<submission_dto::queued_submission>{std::move(*lease_submission_exp)};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> submission_core_service::finalize_submission(
    db_connection& connection,
    std::int64_t submission_id,
    submission_status to_status,
    std::optional<std::int16_t> score_opt,
    std::optional<std::string> compile_output_opt,
    std::optional<std::string> judge_output_opt,
    std::optional<std::string> reason_opt
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto finalize_submission_exp = submission_util::finalize_submission(
            transaction,
            submission_id,
            to_status,
            score_opt,
            compile_output_opt,
            judge_output_opt,
            reason_opt
        );
        if(!finalize_submission_exp){
            return std::unexpected(finalize_submission_exp.error());
        }

        if(finalize_submission_exp->should_increase_accepted_count){
            const auto increase_accepted_count_exp =
                problem_statistics_util::increase_accepted_count(
                    transaction,
                    finalize_submission_exp->problem_id
                );
            if(!increase_accepted_count_exp){
                return std::unexpected(increase_accepted_count_exp.error());
            }
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::vector<submission_dto::summary>, error_code>
submission_core_service::list_submissions(
    db_connection& connection,
    const submission_dto::list_filter& filter_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::read_transaction transaction(connection.connection());
        const auto summary_values_exp = submission_util::list_submissions(
            transaction,
            filter_value
        );
        if(!summary_values_exp){
            return std::unexpected(summary_values_exp.error());
        }

        return *summary_values_exp;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
