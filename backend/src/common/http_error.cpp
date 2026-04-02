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
        case http_error::internal:
            return "internal";
    }
    return "unknown http error";
}

std::optional<http_error> map_error_to_http_error(repository_error ec){
    switch(ec){
        case repository_error::invalid_reference:
        case repository_error::invalid_input:
            return http_error::validation_error;
        case repository_error::not_found:
            return http_error::not_found;
        case repository_error::conflict:
            return http_error::conflict;
        case repository_error::internal:
            return http_error::internal;
    }
    return std::nullopt;
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
    if(ec == http_error::internal){
        return http_error::internal;
    }
    if(ec == psql_error::unique_violation){
        return http_error::conflict;
    }
    if(ec.type_ == error_type::repository_type){
        return map_error_to_http_error(static_cast<repository_error>(ec.code_));
    }

    return std::nullopt;
}
