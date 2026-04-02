#include "error/service_error.hpp"

#include "error/error_code.hpp"

#include <utility>

namespace{
    std::string default_message(service_error_code ec){
        switch(ec){
            case service_error_code::validation_error:
                return "validation error";
            case service_error_code::unauthorized:
                return "unauthorized";
            case service_error_code::forbidden:
                return "forbidden";
            case service_error_code::not_found:
                return "not found";
            case service_error_code::conflict:
                return "conflict";
            case service_error_code::unavailable:
                return "unavailable";
            case service_error_code::internal:
                return "internal";
        }

        return "unknown service error";
    }
}

service_error::service_error(
    service_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? default_message(code_value)
            : std::move(message_value)
    ){}

bool service_error::operator==(const service_error& other) const{
    return code == other.code;
}

const service_error service_error::validation_error{
    service_error_code::validation_error
};
const service_error service_error::unauthorized{
    service_error_code::unauthorized
};
const service_error service_error::forbidden{
    service_error_code::forbidden
};
const service_error service_error::not_found{
    service_error_code::not_found
};
const service_error service_error::conflict{
    service_error_code::conflict
};
const service_error service_error::unavailable{
    service_error_code::unavailable
};
const service_error service_error::internal{
    service_error_code::internal
};

std::string to_string(service_error_code ec){
    switch(ec){
        case service_error_code::validation_error:
            return "validation error";
        case service_error_code::unauthorized:
            return "unauthorized";
        case service_error_code::forbidden:
            return "forbidden";
        case service_error_code::not_found:
            return "not found";
        case service_error_code::conflict:
            return "conflict";
        case service_error_code::unavailable:
            return "unavailable";
        case service_error_code::internal:
            return "internal";
    }

    return "unknown service error";
}

std::string to_string(const service_error& ec){
    return ec.message;
}

std::optional<service_error> service_error::from_repository(
    const repository_error& ec
){
    switch(ec.code){
        case repository_error_code::invalid_reference:
        case repository_error_code::invalid_input:
            return service_error{service_error_code::validation_error, ec.message};
        case repository_error_code::not_found:
            return service_error{service_error_code::not_found, ec.message};
        case repository_error_code::conflict:
            return service_error{service_error_code::conflict, ec.message};
        case repository_error_code::internal:
            return service_error{service_error_code::internal, ec.message};
    }

    return std::nullopt;
}

std::optional<service_error> service_error::from_error_code(const error_code& ec){
    if(ec == http_error::validation_error){
        return service_error::validation_error;
    }
    if(ec == http_error::unauthorized){
        return service_error::unauthorized;
    }
    if(ec == http_error::forbidden){
        return service_error::forbidden;
    }
    if(ec == http_error::not_found){
        return service_error::not_found;
    }
    if(ec == http_error::conflict){
        return service_error::conflict;
    }
    if(ec == http_error::internal){
        return service_error::internal;
    }
    if(ec == errno_error::permission_denied){
        return service_error::forbidden;
    }
    if(
        ec == errno_error::resource_temporarily_unavailable ||
        ec == boost_error::timed_out ||
        ec == boost_error::would_block ||
        ec == boost_error::try_again
    ){
        return service_error::unavailable;
    }
    if(ec == psql_error::unique_violation){
        return service_error::conflict;
    }
    if(ec == errno_error::invalid_argument){
        return service_error::validation_error;
    }
    if(ec.type_ == error_type::repository_type){
        return service_error::from_repository(
            repository_error{static_cast<repository_error_code>(ec.code_)}
        );
    }

    return std::nullopt;
}
