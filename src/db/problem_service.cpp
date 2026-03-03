#include "db/problem_service.hpp"

#include <pqxx/pqxx>

#include <utility>

std::expected<problem_service, error_code> problem_service::create(db_connection db_connection){
    if(!db_connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return problem_service(std::move(db_connection));
}

problem_service::problem_service(db_connection connection) :
    db_connection_(std::move(connection)){}

pqxx::connection& problem_service::connection(){
    return db_connection_.connection();
}

const pqxx::connection& problem_service::connection() const{
    return db_connection_.connection();
}

std::expected<problem_create_response, error_code> problem_service::create_problem(){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    try{
        pqxx::work transaction(connection());
        const auto create_problem_result = transaction.exec_params(
            "INSERT INTO problems(version) "
            "VALUES($1) "
            "RETURNING problem_id, version",
            1
        );

        if(create_problem_result.empty()){
            return std::unexpected(error_code::create(errno_error::unknown_error));
        }

        problem_create_response create_response;
        create_response.problem_id = create_problem_result[0][0].as<std::int64_t>();
        create_response.version = create_problem_result[0][1].as<std::int32_t>();

        transaction.commit();
        return create_response;
    }
    catch(const std::exception& exception){
        return std::unexpected(error_code::map_psql_error_code(exception));
    }
}

std::expected<void, error_code> problem_service::set_problem_version(
    std::int64_t problem_id, std::int32_t version
){
    if(!db_connection_.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }
    if(problem_id <= 0 || version <= 0){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    try{
        pqxx::work transaction(connection());
        const auto update_result = transaction.exec_params(
            "UPDATE problems "
            "SET version = $2 "
            "WHERE problem_id = $1",
            problem_id,
            version
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
