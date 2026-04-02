#include "error/repository_error.hpp"

#include "error/error_code.hpp"

std::string to_string(repository_error ec){
    switch(ec){
        case repository_error::invalid_reference:
            return "invalid reference";
        case repository_error::invalid_input:
            return "invalid input";
        case repository_error::not_found:
            return "repository not found";
        case repository_error::conflict:
            return "repository conflict";
        case repository_error::internal:
            return "repository internal";
    }
    return "unknown repository error";
}

std::optional<repository_error> map_error_to_repository_error(const error_code& ec){
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

    return std::nullopt;
}
