#include "error/pool_error.hpp"

#include "error/db_error.hpp"

#include <utility>

namespace{
    std::string default_message(pool_error_code error){
        switch(error){
            case pool_error_code::invalid_argument:
                return "invalid connection pool argument";
            case pool_error_code::timed_out:
                return "connection pool acquire timed out";
            case pool_error_code::unavailable:
                return "connection pool unavailable";
            case pool_error_code::internal:
                return "connection pool internal error";
        }

        return "unknown connection pool error";
    }

    pool_error map_db_error(const db_error& error){
        switch(error.code){
            case db_error_code::invalid_argument:
                return pool_error{
                    pool_error_code::invalid_argument,
                    error.message
                };
            case db_error_code::invalid_connection:
            case db_error_code::interrupted:
            case db_error_code::broken_connection:
            case db_error_code::serialization_failure:
            case db_error_code::deadlock_detected:
            case db_error_code::unavailable:
                return pool_error{
                    pool_error_code::unavailable,
                    error.message
                };
            case db_error_code::unique_violation:
            case db_error_code::constraint_violation:
            case db_error_code::internal:
                return pool_error{
                    pool_error_code::internal,
                    error.message
                };
        }

        return pool_error::internal;
    }
}

pool_error::pool_error(
    pool_error_code code_value,
    std::string message_value
) :
    code(code_value),
    message(
        message_value.empty()
            ? default_message(code_value)
            : std::move(message_value)
    ){}

pool_error::pool_error(const db_error& error) :
    pool_error(map_db_error(error)){}

bool pool_error::operator==(const pool_error& other) const{
    return code == other.code;
}

const pool_error pool_error::invalid_argument{
    pool_error_code::invalid_argument
};
const pool_error pool_error::timed_out{
    pool_error_code::timed_out
};
const pool_error pool_error::unavailable{
    pool_error_code::unavailable
};
const pool_error pool_error::internal{
    pool_error_code::internal
};

std::string to_string(pool_error_code error){
    switch(error){
        case pool_error_code::invalid_argument:
            return "invalid connection pool argument";
        case pool_error_code::timed_out:
            return "connection pool acquire timed out";
        case pool_error_code::unavailable:
            return "connection pool unavailable";
        case pool_error_code::internal:
            return "connection pool internal error";
    }

    return "unknown connection pool error";
}

std::string to_string(const pool_error& error){
    return error.message;
}
