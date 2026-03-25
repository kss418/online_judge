#include "db_util/db_util.hpp"

bool db_util::should_retry_db_error(const error_code& error_code_value){
    return
        error_code_value == errno_error::invalid_file_descriptor ||
        error_code_value == errno_error::interrupted_system_call ||
        error_code_value == psql_error::broken_connection ||
        error_code_value == psql_error::serialization_failure ||
        error_code_value == psql_error::deadlock_detected;
}

bool db_util::should_reconnect_db_error(const error_code& error_code_value){
    return
        error_code_value == errno_error::invalid_file_descriptor ||
        error_code_value == psql_error::broken_connection;
}
