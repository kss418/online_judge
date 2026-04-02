#include "error/pool_error.hpp"

#include "error/db_error.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace{
    struct pool_error_spec{
        std::string_view default_message;
    };

    constexpr pool_error_spec unknown_pool_error_spec{
        "unknown connection pool error"
    };

    constexpr std::array<pool_error_spec, 4> pool_error_specs{{
        {"invalid connection pool argument"},
        {"connection pool acquire timed out"},
        {"connection pool unavailable"},
        {"connection pool internal error"},
    }};

    const pool_error_spec& describe_pool_error(pool_error_code error){
        const auto index = static_cast<std::size_t>(error);
        if(index >= pool_error_specs.size()){
            return unknown_pool_error_spec;
        }

        return pool_error_specs[index];
    }

    pool_error map_db_error(const db_error& error){
        switch(error.code){
            case db_error_code::invalid_argument:
                return pool_error{
                    pool_error_code::invalid_argument,
                    error.message
                };
            case db_error_code::invalid_connection:
            case db_error_code::interrupted:
            case db_error_code::broken_connection:
            case db_error_code::serialization_failure:
            case db_error_code::deadlock_detected:
            case db_error_code::unavailable:
                return pool_error{
                    pool_error_code::unavailable,
                    error.message
                };
            case db_error_code::unique_violation:
            case db_error_code::constraint_violation:
            case db_error_code::internal:
                return pool_error{
                    pool_error_code::internal,
                    error.message
                };
        }

        return pool_error::internal;
    }
}

pool_error::pool_error(
    pool_error_code code_value,
    std::string message_value
) :
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_pool_error(code_value).default_message}
            : std::move(message_value)
    ){}

pool_error::pool_error(const db_error& error) :
    pool_error(map_db_error(error)){}

bool pool_error::operator==(const pool_error& other) const{
    return code == other.code;
}

const pool_error pool_error::invalid_argument{
    pool_error_code::invalid_argument
};
const pool_error pool_error::timed_out{
    pool_error_code::timed_out
};
const pool_error pool_error::unavailable{
    pool_error_code::unavailable
};
const pool_error pool_error::internal{
    pool_error_code::internal
};

std::string to_string(pool_error_code error){
    return std::string{describe_pool_error(error).default_message};
}

std::string to_string(const pool_error& error){
    return error.message;
}
