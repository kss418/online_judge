#include "error/judge_error.hpp"

#include "error/db_error.hpp"
#include "error/error_code.hpp"
#include "error/infra_error.hpp"

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

judge_error::judge_error(const db_error& ec) :
    judge_error(from_db_error(ec)){}

judge_error::judge_error(const service_error& ec) :
    judge_error(from_service(ec)){}

judge_error::judge_error(const error_code& ec) :
    judge_error(from_error_code(ec)){}

judge_error::judge_error(const infra_error& ec) :
    judge_error(from_infra_error(ec)){}

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

judge_error judge_error::from_db_error(const db_error& ec){
    switch(ec.code){
        case db_error_code::invalid_argument:
        case db_error_code::constraint_violation:
            return judge_error{
                judge_error_code::validation_error,
                ec.message
            };
        case db_error_code::unique_violation:
            return judge_error{
                judge_error_code::conflict,
                ec.message
            };
        case db_error_code::invalid_connection:
        case db_error_code::interrupted:
        case db_error_code::broken_connection:
        case db_error_code::serialization_failure:
        case db_error_code::deadlock_detected:
        case db_error_code::unavailable:
            return judge_error{
                judge_error_code::unavailable,
                ec.message
            };
        case db_error_code::internal:
            return judge_error{
                judge_error_code::internal,
                ec.message
            };
    }

    return judge_error::internal;
}

judge_error judge_error::from_infra_error(const infra_error& ec){
    switch(ec.code){
        case infra_error_code::invalid_argument:
            return judge_error{
                judge_error_code::validation_error,
                ec.message
            };
        case infra_error_code::not_found:
            return judge_error{
                judge_error_code::not_found,
                ec.message
            };
        case infra_error_code::conflict:
            return judge_error{
                judge_error_code::conflict,
                ec.message
            };
        case infra_error_code::unavailable:
            return judge_error{
                judge_error_code::unavailable,
                ec.message
            };
        case infra_error_code::permission_denied:
        case infra_error_code::internal:
            return judge_error{
                judge_error_code::internal,
                ec.message
            };
    }

    return judge_error::internal;
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
    const auto mapped_db_error = db_error::from_error_code(ec);
    if(mapped_db_error != db_error::internal){
        return judge_error::from_db_error(mapped_db_error);
    }

    const auto mapped_infra_error = infra_error::from_error_code(ec);
    if(mapped_infra_error != infra_error::internal){
        return judge_error::from_infra_error(mapped_infra_error);
    }

    return judge_error::internal;
}
