#include "error/infra_error.hpp"

#include "error/error_code.hpp"

#include <utility>

namespace{
    std::string default_message(infra_error_code ec){
        switch(ec){
            case infra_error_code::invalid_argument:
                return "invalid infrastructure argument";
            case infra_error_code::permission_denied:
                return "infrastructure permission denied";
            case infra_error_code::not_found:
                return "infrastructure resource not found";
            case infra_error_code::conflict:
                return "infrastructure conflict";
            case infra_error_code::unavailable:
                return "infrastructure unavailable";
            case infra_error_code::internal:
                return "infrastructure internal error";
        }

        return "unknown infrastructure error";
    }
}

infra_error::infra_error(
    infra_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? default_message(code_value)
            : std::move(message_value)
    ){}

bool infra_error::operator==(const infra_error& other) const{
    return code == other.code;
}

bool infra_error::is_transient() const{
    return *this == infra_error::unavailable;
}

const infra_error infra_error::invalid_argument{
    infra_error_code::invalid_argument
};
const infra_error infra_error::permission_denied{
    infra_error_code::permission_denied
};
const infra_error infra_error::not_found{
    infra_error_code::not_found
};
const infra_error infra_error::conflict{
    infra_error_code::conflict
};
const infra_error infra_error::unavailable{
    infra_error_code::unavailable
};
const infra_error infra_error::internal{
    infra_error_code::internal
};

std::string to_string(infra_error_code ec){
    switch(ec){
        case infra_error_code::invalid_argument:
            return "invalid infrastructure argument";
        case infra_error_code::permission_denied:
            return "infrastructure permission denied";
        case infra_error_code::not_found:
            return "infrastructure resource not found";
        case infra_error_code::conflict:
            return "infrastructure conflict";
        case infra_error_code::unavailable:
            return "infrastructure unavailable";
        case infra_error_code::internal:
            return "infrastructure internal error";
    }

    return "unknown infrastructure error";
}

std::string to_string(const infra_error& ec){
    return ec.message;
}

infra_error infra_error::from_error_code(const error_code& ec){
    if(ec == errno_error::invalid_argument){
        return infra_error::invalid_argument;
    }
    if(ec == errno_error::permission_denied){
        return infra_error::permission_denied;
    }
    if(ec == errno_error::file_not_found){
        return infra_error::not_found;
    }
    if(ec == errno_error::file_exists){
        return infra_error::conflict;
    }
    if(
        ec == errno_error::resource_temporarily_unavailable ||
        ec == boost_error::timed_out ||
        ec == boost_error::would_block ||
        ec == boost_error::try_again
    ){
        return infra_error::unavailable;
    }

    return infra_error::internal;
}
