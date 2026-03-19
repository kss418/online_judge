#include "db_service/problem_content_service.hpp"
#include "db_util/problem_content_util.hpp"
#include "db_util/problem_core_util.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<problem_dto::statement, error_code> problem_content_service::get_statement(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::read_transaction transaction(connection.connection());
        const auto statement_exp = problem_content_util::get_statement(transaction, problem_id);
        if(!statement_exp){
            return std::unexpected(statement_exp.error());
        }

        return *statement_exp;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_content_service::set_statement(
    db_connection& connection,
    std::int64_t problem_id,
    const problem_dto::statement& statement
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto set_statement_exp = problem_content_util::set_statement(
            transaction,
            problem_id,
            statement
        );
        if(!set_statement_exp){
            return std::unexpected(set_statement_exp.error());
        }

        const auto version_exp = problem_core_util::increase_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::vector<problem_dto::sample>, error_code> problem_content_service::list_samples(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::read_transaction transaction(connection.connection());
        const auto sample_values_exp = problem_content_util::list_samples(transaction, problem_id);
        if(!sample_values_exp){
            return std::unexpected(sample_values_exp.error());
        }

        return *sample_values_exp;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
