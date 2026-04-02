#include "db_repository/db_repository.hpp"

bool db_repository::should_retry_db_error(const error_code& error_code_value){
    return
        error_code_value == errno_error::invalid_file_descriptor ||
        error_code_value == errno_error::interrupted_system_call ||
        error_code_value == psql_error::broken_connection ||
        error_code_value == psql_error::serialization_failure ||
        error_code_value == psql_error::deadlock_detected;
}

bool db_repository::should_reconnect_db_error(const error_code& error_code_value){
    return
        error_code_value == errno_error::invalid_file_descriptor ||
        error_code_value == psql_error::broken_connection;
}

repository_error db_repository::invalid_reference_error(){
    return repository_error::invalid_reference;
}

repository_error db_repository::invalid_input_error(){
    return repository_error::invalid_input;
}

repository_error db_repository::not_found_error(){
    return repository_error::not_found;
}

repository_error db_repository::conflict_error(){
    return repository_error::conflict;
}

repository_error db_repository::internal_error(){
    return repository_error::internal;
}

repository_error db_repository::map_error(const error_code& error_code_value){
    if(const auto repository_error_opt = repository_error::from_error_code(error_code_value)){
        return *repository_error_opt;
    }

    return internal_error();
}
