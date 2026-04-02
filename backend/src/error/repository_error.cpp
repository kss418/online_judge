#include "error/repository_error.hpp"

#include "error/db_error.hpp"
#include "error/error_code.hpp"

#include <utility>

namespace{
    std::string default_message(repository_error_code ec){
        switch(ec){
            case repository_error_code::invalid_reference:
                return "invalid reference";
            case repository_error_code::invalid_input:
                return "invalid input";
            case repository_error_code::not_found:
                return "repository not found";
            case repository_error_code::conflict:
                return "repository conflict";
            case repository_error_code::internal:
                return "repository internal";
        }
        return "unknown repository error";
    }
}

repository_error::repository_error(
    repository_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? default_message(code_value)
            : std::move(message_value)
    ){}

repository_error::repository_error(const db_error& ec)
:
    repository_error(from_db_error(ec)){}

bool repository_error::operator==(const repository_error& other) const{
    return code == other.code;
}

const repository_error repository_error::invalid_reference{
    repository_error_code::invalid_reference
};
const repository_error repository_error::invalid_input{
    repository_error_code::invalid_input
};
const repository_error repository_error::not_found{
    repository_error_code::not_found
};
const repository_error repository_error::conflict{
    repository_error_code::conflict
};
const repository_error repository_error::internal{
    repository_error_code::internal
};

std::string to_string(repository_error_code ec){
    switch(ec){
        case repository_error_code::invalid_reference:
            return "invalid reference";
        case repository_error_code::invalid_input:
            return "invalid input";
        case repository_error_code::not_found:
            return "repository not found";
        case repository_error_code::conflict:
            return "repository conflict";
        case repository_error_code::internal:
            return "repository internal";
    }
    return "unknown repository error";
}

std::string to_string(const repository_error& ec){
    return ec.message;
}

repository_error repository_error::from_db_error(const db_error& ec){
    switch(ec.code){
        case db_error_code::invalid_argument:
        case db_error_code::constraint_violation:
            return repository_error{
                repository_error_code::invalid_input,
                ec.message
            };
        case db_error_code::unique_violation:
            return repository_error{
                repository_error_code::conflict,
                ec.message
            };
        case db_error_code::invalid_connection:
        case db_error_code::interrupted:
        case db_error_code::broken_connection:
        case db_error_code::serialization_failure:
        case db_error_code::deadlock_detected:
        case db_error_code::unavailable:
        case db_error_code::internal:
            return repository_error{
                repository_error_code::internal,
                ec.message
            };
    }

    return repository_error::internal;
}

repository_error repository_error::from_error_code(const error_code& ec){
    return from_db_error(db_error::from_error_code(ec));
}
