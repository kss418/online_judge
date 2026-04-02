#include "error/http_error.hpp"

#include "error/error_code.hpp"

#include <utility>

namespace{
    std::string default_message(http_error_code ec){
        switch(ec){
            case http_error_code::validation_error:
                return "validation error";
            case http_error_code::unauthorized:
                return "unauthorized";
            case http_error_code::forbidden:
                return "forbidden";
            case http_error_code::not_found:
                return "not found";
            case http_error_code::conflict:
                return "conflict";
            case http_error_code::internal:
                return "internal";
        }
        return "unknown http error";
    }
}

http_error::http_error(
    http_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? default_message(code_value)
            : std::move(message_value)
    ){}

bool http_error::operator==(const http_error& other) const{
    return code == other.code;
}

const http_error http_error::validation_error{
    http_error_code::validation_error
};
const http_error http_error::unauthorized{
    http_error_code::unauthorized
};
const http_error http_error::forbidden{
    http_error_code::forbidden
};
const http_error http_error::not_found{
    http_error_code::not_found
};
const http_error http_error::conflict{
    http_error_code::conflict
};
const http_error http_error::internal{
    http_error_code::internal
};

std::string to_string(http_error_code ec){
    switch(ec){
        case http_error_code::validation_error:
            return "validation error";
        case http_error_code::unauthorized:
            return "unauthorized";
        case http_error_code::forbidden:
            return "forbidden";
        case http_error_code::not_found:
            return "not found";
        case http_error_code::conflict:
            return "conflict";
        case http_error_code::internal:
            return "internal";
    }
    return "unknown http error";
}

std::string to_string(const http_error& ec){
    return ec.message;
}

http_error http_error::from_service(const service_error& ec){
    switch(ec.code){
        case service_error_code::validation_error:
            return http_error{http_error_code::validation_error, ec.message};
        case service_error_code::unauthorized:
            return http_error{http_error_code::unauthorized, ec.message};
        case service_error_code::forbidden:
            return http_error{http_error_code::forbidden, ec.message};
        case service_error_code::not_found:
            return http_error{http_error_code::not_found, ec.message};
        case service_error_code::conflict:
            return http_error{http_error_code::conflict, ec.message};
        case service_error_code::unavailable:
        case service_error_code::internal:
            return http_error{http_error_code::internal, ec.message};
    }

    return http_error::internal;
}

http_error http_error::from_error_code(const error_code& ec){
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

    return http_error::from_service(service_error::from_error_code(ec));
}
