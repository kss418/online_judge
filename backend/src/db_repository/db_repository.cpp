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

error_code db_repository::invalid_reference_error(){
    return error_code::create(repository_error::invalid_reference);
}

error_code db_repository::invalid_input_error(){
    return error_code::create(repository_error::invalid_input);
}

error_code db_repository::not_found_error(){
    return error_code::create(repository_error::not_found);
}

error_code db_repository::conflict_error(){
    return error_code::create(repository_error::conflict);
}
