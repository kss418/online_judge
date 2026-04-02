#include "error/judge_error.hpp"

#include "error/error_code.hpp"

#include <utility>

namespace{
    std::string default_message(judge_error_code ec){
        switch(ec){
            case judge_error_code::validation_error:
                return "validation error";
            case judge_error_code::not_found:
                return "not found";
            case judge_error_code::conflict:
                return "conflict";
            case judge_error_code::unavailable:
                return "unavailable";
            case judge_error_code::internal:
                return "internal";
        }

        return "unknown judge error";
    }
}

judge_error::judge_error(
    judge_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? default_message(code_value)
            : std::move(message_value)
    ){}

bool judge_error::operator==(const judge_error& other) const{
    return code == other.code;
}

const judge_error judge_error::validation_error{
    judge_error_code::validation_error
};
const judge_error judge_error::not_found{
    judge_error_code::not_found
};
const judge_error judge_error::conflict{
    judge_error_code::conflict
};
const judge_error judge_error::unavailable{
    judge_error_code::unavailable
};
const judge_error judge_error::internal{
    judge_error_code::internal
};

std::string to_string(judge_error_code ec){
    switch(ec){
        case judge_error_code::validation_error:
            return "validation error";
        case judge_error_code::not_found:
            return "not found";
        case judge_error_code::conflict:
            return "conflict";
        case judge_error_code::unavailable:
            return "unavailable";
        case judge_error_code::internal:
            return "internal";
    }

    return "unknown judge error";
}

std::string to_string(const judge_error& ec){
    return ec.message;
}

judge_error judge_error::from_service(const service_error& ec){
    switch(ec.code){
        case service_error_code::validation_error:
            return judge_error{judge_error_code::validation_error, ec.message};
        case service_error_code::not_found:
            return judge_error{judge_error_code::not_found, ec.message};
        case service_error_code::conflict:
            return judge_error{judge_error_code::conflict, ec.message};
        case service_error_code::unavailable:
            return judge_error{judge_error_code::unavailable, ec.message};
        case service_error_code::unauthorized:
        case service_error_code::forbidden:
        case service_error_code::internal:
            return judge_error{judge_error_code::internal, ec.message};
    }

    return judge_error::internal;
}

judge_error judge_error::from_error_code(const error_code& ec){
    if(ec == errno_error::resource_temporarily_unavailable){
        return judge_error::unavailable;
    }
    if(
        ec == boost_error::timed_out ||
        ec == boost_error::would_block ||
        ec == boost_error::try_again
    ){
        return judge_error::unavailable;
    }

    return judge_error::from_service(service_error::from_error_code(ec));
}
