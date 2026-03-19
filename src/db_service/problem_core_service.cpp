#include "db_service/problem_core_service.hpp"
#include "db_util/problem_content_util.hpp"
#include "db_util/problem_core_util.hpp"
#include "db_util/problem_statistics_util.hpp"

#include <pqxx/pqxx>

std::expected<bool, error_code> problem_core_service::exists_problem(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto exists_exp = problem_core_util::exists_problem(transaction, problem_id);
        if(!exists_exp){
            return std::unexpected(exists_exp.error());
        }

        transaction.commit();
        return exists_exp.value();
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::int32_t, error_code> problem_core_service::get_version(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::read_transaction transaction(connection.connection());
        const auto version_exp = problem_core_util::get_version(transaction, problem_id);
        if(!version_exp){
            return std::unexpected(version_exp.error());
        }

        return *version_exp;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<std::int64_t, error_code> problem_core_service::create_problem(db_connection& connection){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto problem_id_exp = problem_core_util::create_problem(
            transaction
        );
        if(!problem_id_exp){
            return std::unexpected(problem_id_exp.error());
        }

        problem_dto::limits initial_limits_value;
        initial_limits_value.memory_mb = problem_core_service::INITIAL_MEMORY_LIMIT_MB;
        initial_limits_value.time_ms = problem_core_service::INITIAL_TIME_LIMIT_MS;

        const auto set_limits_exp = problem_core_util::set_limits(
            transaction,
            *problem_id_exp,
            initial_limits_value
        );
        if(!set_limits_exp){
            return std::unexpected(set_limits_exp.error());
        }

        const auto create_problem_statistics_exp = problem_statistics_util::create_problem_statistics(
            transaction,
            *problem_id_exp
        );
        if(!create_problem_statistics_exp){
            return std::unexpected(create_problem_statistics_exp.error());
        }

        const auto ensure_statement_exp = problem_content_util::ensure_statement_row(
            transaction,
            *problem_id_exp
        );
        if(!ensure_statement_exp){
            return std::unexpected(ensure_statement_exp.error());
        }

        transaction.commit();
        return *problem_id_exp;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<problem_dto::limits, error_code> problem_core_service::get_limits(
    db_connection& connection,
    std::int64_t problem_id
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::read_transaction transaction(connection.connection());
        const auto limits_exp = problem_core_util::get_limits(transaction, problem_id);
        if(!limits_exp){
            return std::unexpected(limits_exp.error());
        }

        return *limits_exp;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_core_service::set_limits(
    db_connection& connection,
    std::int64_t problem_id,
    const problem_dto::limits& limits_value
){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection.connection());
        const auto set_limits_exp = problem_core_util::set_limits(
            transaction,
            problem_id,
            limits_value
        );
        if(!set_limits_exp){
            return std::unexpected(set_limits_exp.error());
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
