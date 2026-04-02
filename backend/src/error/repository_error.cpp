#include "error/repository_error.hpp"

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

repository_error repository_error::from_error_code(const error_code& ec){
    if(ec == repository_error::invalid_reference){
        return repository_error::invalid_reference;
    }
    if(ec == repository_error::invalid_input){
        return repository_error::invalid_input;
    }
    if(ec == repository_error::not_found){
        return repository_error::not_found;
    }
    if(ec == repository_error::conflict){
        return repository_error::conflict;
    }
    if(ec == repository_error::internal){
        return repository_error::internal;
    }
    if(ec == psql_error::unique_violation){
        return repository_error::conflict;
    }
    if(
        ec == errno_error::invalid_argument ||
        ec == http_error::validation_error ||
        (
            ec.is_constraint_violation_error() &&
            ec != psql_error::unique_violation
        )
    ){
        return repository_error::invalid_input;
    }
    if(ec == http_error::not_found){
        return repository_error::not_found;
    }
    if(ec == http_error::conflict){
        return repository_error::conflict;
    }
    if(
        ec == http_error::internal ||
        ec == http_error::unauthorized ||
        ec == http_error::forbidden
    ){
        return repository_error::internal;
    }

    return repository_error::internal;
}
