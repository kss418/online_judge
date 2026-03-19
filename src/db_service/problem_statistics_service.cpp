#include "db_service/problem_statistics_service.hpp"

#include <pqxx/pqxx>

std::expected<problem_dto::statistics, error_code> problem_statistics_service::get_statistics(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto statistics_query_result = transaction.exec(
            "SELECT submission_count, accepted_count "
            "FROM problem_statistics "
            "WHERE problem_id = $1",
            pqxx::params{problem_id}
        );

        if(statistics_query_result.empty()){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        problem_dto::statistics statistics_value;
        statistics_value.submission_count = statistics_query_result[0][0].as<std::int64_t>();
        statistics_value.accepted_count = statistics_query_result[0][1].as<std::int64_t>();
        return statistics_value;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
