#include "common/http_error.hpp"

#include "common/error_code.hpp"

std::string to_string(http_error ec){
    switch(ec){
        case http_error::validation_error:
            return "validation error";
        case http_error::unauthorized:
            return "unauthorized";
        case http_error::forbidden:
            return "forbidden";
        case http_error::not_found:
            return "not found";
        case http_error::conflict:
            return "conflict";
    }
    return "unknown http error";
}

std::optional<http_error> map_error_to_http_error(const error_code& ec){
    if(ec == http_error::validation_error){
        return http_error::validation_error;
    }
    if(ec == http_error::unauthorized){
        return http_error::unauthorized;
    }
    if(ec == http_error::forbidden){
        return http_error::forbidden;
    }
    if(ec == http_error::not_found){
        return http_error::not_found;
    }
    if(ec == http_error::conflict){
        return http_error::conflict;
    }
    if(
        ec == repository_error::invalid_reference ||
        ec == repository_error::invalid_input
    ){
        return http_error::validation_error;
    }
    if(ec == repository_error::not_found){
        return http_error::not_found;
    }
    if(ec == repository_error::conflict || ec == psql_error::unique_violation){
        return http_error::conflict;
    }

    return std::nullopt;
}
