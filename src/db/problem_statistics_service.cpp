#include "db/problem_statistics_service.hpp"

#include <pqxx/pqxx>

#include <utility>

problem_statistics_service::problem_statistics_service(db_connection connection) :
    db_service_base<problem_statistics_service>(std::move(connection)){}

std::expected<void, error_code> problem_statistics_service::increase_submission_count(std::int64_t problem_id){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto update_result = transaction.exec_params(
            "UPDATE problem_statistics "
            "SET "
            "submission_count = submission_count + 1, "
            "updated_at = NOW() "
            "WHERE problem_id = $1",
            problem_id
        );

        if(update_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_statistics_service::increase_accepted_count(std::int64_t problem_id){
    if(!is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto update_result = transaction.exec_params(
            "UPDATE problem_statistics "
            "SET "
            "accepted_count = accepted_count + 1, "
            "updated_at = NOW() "
            "WHERE problem_id = $1",
            problem_id
        );

        if(update_result.affected_rows() == 0){
            return std::unexpected(error_code::create(errno_error::invalid_argument));
        }

        transaction.commit();
        return {};
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}
