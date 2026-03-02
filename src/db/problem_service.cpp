#include "db/problem_service.hpp"

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
