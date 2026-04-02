#include "error/db_error.hpp"

#include "error/error_code.hpp"

#include <utility>

namespace{
    std::string default_message(db_error_code ec){
        switch(ec){
            case db_error_code::invalid_argument:
                return "invalid database argument";
            case db_error_code::invalid_connection:
                return "invalid database connection";
            case db_error_code::interrupted:
                return "database operation interrupted";
            case db_error_code::unique_violation:
                return "database unique violation";
            case db_error_code::constraint_violation:
                return "database constraint violation";
            case db_error_code::broken_connection:
                return "database connection broken";
            case db_error_code::serialization_failure:
                return "database serialization failure";
            case db_error_code::deadlock_detected:
                return "database deadlock detected";
            case db_error_code::unavailable:
                return "database unavailable";
            case db_error_code::internal:
                return "database internal error";
        }

        return "unknown database error";
    }

    db_error map_psql_error(psql_error ec){
        switch(ec){
            case psql_error::serialization_failure:
                return db_error::serialization_failure;
            case psql_error::deadlock_detected:
                return db_error::deadlock_detected;
            case psql_error::unique_violation:
                return db_error::unique_violation;
            case psql_error::foreign_key_violation:
            case psql_error::not_null_violation:
            case psql_error::check_violation:
                return db_error::constraint_violation;
            case psql_error::broken_connection:
                return db_error::broken_connection;
            default:
                return db_error::internal;
        }
    }
}

db_error::db_error(
    db_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? default_message(code_value)
            : std::move(message_value)
    ){}

bool db_error::operator==(const db_error& other) const{
    return code == other.code;
}

bool db_error::is_retryable() const{
    return
        *this == db_error::invalid_connection ||
        *this == db_error::interrupted ||
        *this == db_error::broken_connection ||
        *this == db_error::serialization_failure ||
        *this == db_error::deadlock_detected;
}

bool db_error::should_reconnect() const{
    return
        *this == db_error::invalid_connection ||
        *this == db_error::broken_connection;
}

bool db_error::is_constraint_violation() const{
    return
        *this == db_error::unique_violation ||
        *this == db_error::constraint_violation;
}

const db_error db_error::invalid_argument{
    db_error_code::invalid_argument
};
const db_error db_error::invalid_connection{
    db_error_code::invalid_connection
};
const db_error db_error::interrupted{
    db_error_code::interrupted
};
const db_error db_error::unique_violation{
    db_error_code::unique_violation
};
const db_error db_error::constraint_violation{
    db_error_code::constraint_violation
};
const db_error db_error::broken_connection{
    db_error_code::broken_connection
};
const db_error db_error::serialization_failure{
    db_error_code::serialization_failure
};
const db_error db_error::deadlock_detected{
    db_error_code::deadlock_detected
};
const db_error db_error::unavailable{
    db_error_code::unavailable
};
const db_error db_error::internal{
    db_error_code::internal
};

std::string to_string(db_error_code ec){
    switch(ec){
        case db_error_code::invalid_argument:
            return "invalid database argument";
        case db_error_code::invalid_connection:
            return "invalid database connection";
        case db_error_code::interrupted:
            return "database operation interrupted";
        case db_error_code::unique_violation:
            return "database unique violation";
        case db_error_code::constraint_violation:
            return "database constraint violation";
        case db_error_code::broken_connection:
            return "database connection broken";
        case db_error_code::serialization_failure:
            return "database serialization failure";
        case db_error_code::deadlock_detected:
            return "database deadlock detected";
        case db_error_code::unavailable:
            return "database unavailable";
        case db_error_code::internal:
            return "database internal error";
    }

    return "unknown database error";
}

std::string to_string(const db_error& ec){
    return ec.message;
}

db_error db_error::from_psql_exception(const std::exception& exception){
    return map_psql_error(error_code::map_psql_error(exception));
}
