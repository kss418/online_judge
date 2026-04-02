#include "error/repository_error.hpp"

#include "error/db_error.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace{
    struct repository_error_spec{
        std::string_view default_message;
    };

    constexpr repository_error_spec unknown_repository_error_spec{
        "unknown repository error"
    };

    constexpr std::array<repository_error_spec, 5> repository_error_specs{{
        {"invalid reference"},
        {"invalid input"},
        {"repository not found"},
        {"repository conflict"},
        {"repository internal"},
    }};

    const repository_error_spec& describe_repository_error(repository_error_code ec){
        const auto index = static_cast<std::size_t>(ec);
        if(index >= repository_error_specs.size()){
            return unknown_repository_error_spec;
        }

        return repository_error_specs[index];
    }
}

repository_error::repository_error(
    repository_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_repository_error(code_value).default_message}
            : std::move(message_value)
    ){}

repository_error::repository_error(const db_error& ec)
:
    repository_error(from_db_error(ec)){}

bool repository_error::operator==(const repository_error& other) const{
    return code == other.code;
}

const repository_error repository_error::invalid_reference{
    repository_error_code::invalid_reference
};
const repository_error repository_error::invalid_input{
    repository_error_code::invalid_input
};
const repository_error repository_error::not_found{
    repository_error_code::not_found
};
const repository_error repository_error::conflict{
    repository_error_code::conflict
};
const repository_error repository_error::internal{
    repository_error_code::internal
};

bool repository_error::should_retry_db_error(const db_error& error){
    return error.is_retryable();
}

bool repository_error::should_reconnect_db_error(const db_error& error){
    return error.should_reconnect();
}

std::string to_string(repository_error_code ec){
    return std::string{describe_repository_error(ec).default_message};
}

std::string to_string(const repository_error& ec){
    return ec.message;
}

repository_error repository_error::from_db_error(const db_error& ec){
    switch(ec.code){
        case db_error_code::invalid_argument:
        case db_error_code::constraint_violation:
            return repository_error{
                repository_error_code::invalid_input,
                ec.message
            };
        case db_error_code::unique_violation:
            return repository_error{
                repository_error_code::conflict,
                ec.message
            };
        case db_error_code::invalid_connection:
        case db_error_code::interrupted:
        case db_error_code::broken_connection:
        case db_error_code::serialization_failure:
        case db_error_code::deadlock_detected:
        case db_error_code::unavailable:
        case db_error_code::internal:
            return repository_error{
                repository_error_code::internal,
                ec.message
            };
    }

    return repository_error::internal;
}
