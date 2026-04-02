#include "error/service_error.hpp"

#include "error/error_code.hpp"

std::string to_string(service_error ec){
    switch(ec){
        case service_error::validation_error:
            return "validation error";
        case service_error::unauthorized:
            return "unauthorized";
        case service_error::forbidden:
            return "forbidden";
        case service_error::not_found:
            return "not found";
        case service_error::conflict:
            return "conflict";
        case service_error::unavailable:
            return "unavailable";
        case service_error::internal:
            return "internal";
    }

    return "unknown service error";
}

namespace service_error_util{
    std::optional<service_error> from_repository(repository_error ec){
        switch(ec){
            case repository_error::invalid_reference:
            case repository_error::invalid_input:
                return service_error::validation_error;
            case repository_error::not_found:
                return service_error::not_found;
            case repository_error::conflict:
                return service_error::conflict;
            case repository_error::internal:
                return service_error::internal;
        }

        return std::nullopt;
    }

    std::optional<service_error> from_error_code(const error_code& ec){
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
            return service_error_util::from_repository(
                static_cast<repository_error>(ec.code_)
            );
        }

        return std::nullopt;
    }
}
