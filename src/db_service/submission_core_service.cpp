#include "db_service/submission_core_service.hpp"
#include "db_util/problem_statistics_util.hpp"
#include "db_util/submission_util.hpp"

#include <pqxx/pqxx>

#include <string>
#include <utility>

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
