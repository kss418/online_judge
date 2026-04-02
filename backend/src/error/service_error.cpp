#include "error/service_error.hpp"

#include "error/db_error.hpp"
#include "error/infra_error.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace{
    struct service_error_spec{
        std::string_view default_message;
    };

    constexpr service_error_spec unknown_service_error_spec{
        "unknown service error"
    };

    constexpr std::array<service_error_spec, 7> service_error_specs{{
        {"validation error"},
        {"unauthorized"},
        {"forbidden"},
        {"not found"},
        {"conflict"},
        {"unavailable"},
        {"internal"},
    }};

    const service_error_spec& describe_service_error(service_error_code ec){
        const auto index = static_cast<std::size_t>(ec);
        if(index >= service_error_specs.size()){
            return unknown_service_error_spec;
        }

        return service_error_specs[index];
    }
}

service_error::service_error(
    service_error_code code_value,
    std::string message_value
):
    code(code_value),
    message(
        message_value.empty()
            ? std::string{describe_service_error(code_value).default_message}
            : std::move(message_value)
    ){}

service_error::service_error(const repository_error& ec)
:
    service_error(from_repository(ec)){}

service_error::service_error(const db_error& ec)
:
    service_error(from_db_error(ec)){}

service_error::service_error(const infra_error& ec)
:
    service_error(from_infra_error(ec)){}

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
    return std::string{describe_service_error(ec).default_message};
}

std::string to_string(const service_error& ec){
    return ec.message;
}

service_error service_error::from_db_error(const db_error& ec){
    switch(ec.code){
        case db_error_code::invalid_argument:
        case db_error_code::constraint_violation:
            return service_error{
                service_error_code::validation_error,
                ec.message
            };
        case db_error_code::unique_violation:
            return service_error{
                service_error_code::conflict,
                ec.message
            };
        case db_error_code::invalid_connection:
        case db_error_code::interrupted:
        case db_error_code::broken_connection:
        case db_error_code::serialization_failure:
        case db_error_code::deadlock_detected:
        case db_error_code::unavailable:
            return service_error{
                service_error_code::unavailable,
                ec.message
            };
        case db_error_code::internal:
            return service_error{
                service_error_code::internal,
                ec.message
            };
    }

    return service_error::internal;
}

service_error service_error::from_infra_error(const infra_error& ec){
    switch(ec.code){
        case infra_error_code::invalid_argument:
            return service_error{
                service_error_code::validation_error,
                ec.message
            };
        case infra_error_code::permission_denied:
            return service_error{
                service_error_code::forbidden,
                ec.message
            };
        case infra_error_code::not_found:
            return service_error{
                service_error_code::not_found,
                ec.message
            };
        case infra_error_code::conflict:
            return service_error{
                service_error_code::conflict,
                ec.message
            };
        case infra_error_code::unavailable:
            return service_error{
                service_error_code::unavailable,
                ec.message
            };
        case infra_error_code::internal:
            return service_error{
                service_error_code::internal,
                ec.message
            };
    }

    return service_error::internal;
}

service_error service_error::from_repository(const repository_error& ec){
    switch(ec.code){
        case repository_error_code::invalid_reference:
        case repository_error_code::invalid_input:
            return service_error{
                service_error_code::validation_error,
                ec.message
            };
        case repository_error_code::not_found:
            return service_error{service_error_code::not_found, ec.message};
        case repository_error_code::conflict:
            return service_error{service_error_code::conflict, ec.message};
        case repository_error_code::internal:
            return service_error{service_error_code::internal, ec.message};
    }

    return service_error::internal;
}
