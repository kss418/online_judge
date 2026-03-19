#include "db_service/submission_service.hpp"
#include "db_util/problem_statistics_util.hpp"
#include "db_util/submission_util.hpp"

#include <pqxx/pqxx>

#include <string>
#include <utility>

static bool is_queue_empty_error(const error_code& code){
    return code == errno_error::resource_temporarily_unavailable;
}

std::expected<submission_dto::created, error_code> submission_service::create_submission(
    db_connection& connection,
    const submission_dto::create_request& create_request_value
){
    problem_dto::reference problem_reference_value;
    problem_reference_value.problem_id = create_request_value.problem_id;
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(
        create_request_value.user_id <= 0 ||
        problem_reference_value.problem_id <= 0 ||
        create_request_value.source_value.language.empty() ||
        create_request_value.source_value.source_code.empty()
    ){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto create_submission_exp = submission_util::create_submission(
            transaction,
            create_request_value
        );
        if(!create_submission_exp){
            return std::unexpected(create_submission_exp.error());
        }

        const auto increase_submission_count_exp = problem_statistics_util::increase_submission_count(
            transaction,
            problem_reference_value
        );
        if(!increase_submission_count_exp){
            return std::unexpected(increase_submission_count_exp.error());
        }

        transaction.commit();
        return *create_submission_exp;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> submission_service::update_submission_status(
    db_connection& connection,
    const submission_dto::status_update& status_update_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto update_submission_status_exp = submission_util::update_submission_status(
            transaction,
            status_update_value
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
submission_service::lease_submission(
    db_connection& connection,
    const submission_dto::lease_request& lease_request_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        auto lease_submission_exp = submission_util::lease_submission(
            transaction,
            lease_request_value
        );
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

std::expected<void, error_code> submission_service::finalize_submission(
    db_connection& connection,
    const submission_dto::finalize_request& finalize_request_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto finalize_submission_exp = submission_util::finalize_submission(
            transaction,
            finalize_request_value
        );
        if(!finalize_submission_exp){
            return std::unexpected(finalize_submission_exp.error());
        }

        problem_dto::reference problem_reference_value;
        problem_reference_value.problem_id = finalize_submission_exp->problem_id;
        if(finalize_submission_exp->should_increase_accepted_count){
            const auto increase_accepted_count_exp =
                problem_statistics_util::increase_accepted_count(
                    transaction,
                    problem_reference_value
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
submission_service::list_submissions(
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
