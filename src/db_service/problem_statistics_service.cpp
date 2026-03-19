#include "db_service/problem_statistics_service.hpp"
#include "db_util/problem_statistics_util.hpp"

#include <pqxx/pqxx>

std::expected<problem_dto::statistics, error_code> problem_statistics_service::get_statistics(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::read_transaction transaction(connection.connection());
        const auto statistics_exp = problem_statistics_util::get_statistics(
            transaction,
            problem_id
        );
        if(!statistics_exp){
            return std::unexpected(statistics_exp.error());
        }

        return *statistics_exp;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
